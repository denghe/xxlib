#include "pkg_game_client.h"
#include "pkg_game_client.cpp.inc"
void CodeGen_pkg_game_client::Register() {
	::xx::ObjManager::Register<::Game1::Event>();
	::xx::ObjManager::Register<::Game1::Player>();
	::xx::ObjManager::Register<::Game1::Message>();
	::xx::ObjManager::Register<::Game1::Scene>();
	::xx::ObjManager::Register<::Game1_Client::FullSync>();
	::xx::ObjManager::Register<::Game1_Client::Sync>();
	::xx::ObjManager::Register<::Client_Game1::Enter>();
	::xx::ObjManager::Register<::Client_Game1::Leave>();
	::xx::ObjManager::Register<::Client_Game1::SendMessage>();
	::xx::ObjManager::Register<::Game1::Event_PlayerEnter>();
	::xx::ObjManager::Register<::Game1::Event_PlayerLeave>();
	::xx::ObjManager::Register<::Game1::Event_Message>();
}
namespace Game1{
    void Event::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->timestamp);
    }
    int Event::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->timestamp)) return r;
        return 0;
    }
    void Event::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":70");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Event::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"timestamp\":", this->timestamp);
#endif
    }
    void Event::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Game1::Event*)tar;
        om.Clone_(this->timestamp, out->timestamp);
    }
    int Event::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->timestamp)) return r;
        return 0;
    }
    void Event::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->timestamp);
    }
    void Event::SetDefaultValue(::xx::ObjManager& om) {
        this->timestamp = 0;
    }
}
namespace Game1{
    void Player::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->info);
    }
    int Player::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->info)) return r;
        return 0;
    }
    void Player::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":50");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Player::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"info\":", this->info);
#endif
    }
    void Player::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Game1::Player*)tar;
        om.Clone_(this->info, out->info);
    }
    int Player::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->info)) return r;
        return 0;
    }
    void Player::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->info);
    }
    void Player::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->info);
    }
}
namespace Game1{
    void Message::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->timestamp);
        om.Write(d, this->senderId);
        om.Write(d, this->senderNickname);
        om.Write(d, this->content);
    }
    int Message::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->timestamp)) return r;
        if (int r = om.Read(d, this->senderId)) return r;
        if (int r = om.Read(d, this->senderNickname)) return r;
        if (int r = om.Read(d, this->content)) return r;
        return 0;
    }
    void Message::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":51");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Message::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"timestamp\":", this->timestamp);
        om.Append(s, ",\"senderId\":", this->senderId);
        om.Append(s, ",\"senderNickname\":", this->senderNickname);
        om.Append(s, ",\"content\":", this->content);
#endif
    }
    void Message::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Game1::Message*)tar;
        om.Clone_(this->timestamp, out->timestamp);
        om.Clone_(this->senderId, out->senderId);
        om.Clone_(this->senderNickname, out->senderNickname);
        om.Clone_(this->content, out->content);
    }
    int Message::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->timestamp)) return r;
        if (int r = om.RecursiveCheck(this->senderId)) return r;
        if (int r = om.RecursiveCheck(this->senderNickname)) return r;
        if (int r = om.RecursiveCheck(this->content)) return r;
        return 0;
    }
    void Message::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->timestamp);
        om.RecursiveReset(this->senderId);
        om.RecursiveReset(this->senderNickname);
        om.RecursiveReset(this->content);
    }
    void Message::SetDefaultValue(::xx::ObjManager& om) {
        this->timestamp = 0;
        this->senderId = 0;
        om.SetDefaultValue(this->senderNickname);
        om.SetDefaultValue(this->content);
    }
}
namespace Game1{
    void Scene::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->players);
        om.Write(d, this->messages);
    }
    int Scene::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->players)) return r;
        if (int r = om.Read(d, this->messages)) return r;
        return 0;
    }
    void Scene::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":52");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Scene::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"players\":", this->players);
        om.Append(s, ",\"messages\":", this->messages);
#endif
    }
    void Scene::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Game1::Scene*)tar;
        om.Clone_(this->players, out->players);
        om.Clone_(this->messages, out->messages);
    }
    int Scene::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->players)) return r;
        if (int r = om.RecursiveCheck(this->messages)) return r;
        return 0;
    }
    void Scene::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->players);
        om.RecursiveReset(this->messages);
    }
    void Scene::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->players);
        om.SetDefaultValue(this->messages);
    }
}
namespace Game1_Client{
    void FullSync::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->scene);
        om.Write(d, this->self);
    }
    int FullSync::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->scene)) return r;
        if (int r = om.Read(d, this->self)) return r;
        return 0;
    }
    void FullSync::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":80");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void FullSync::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"scene\":", this->scene);
        om.Append(s, ",\"self\":", this->self);
#endif
    }
    void FullSync::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Game1_Client::FullSync*)tar;
        om.Clone_(this->scene, out->scene);
        om.Clone_(this->self, out->self);
    }
    int FullSync::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->scene)) return r;
        if (int r = om.RecursiveCheck(this->self)) return r;
        return 0;
    }
    void FullSync::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->scene);
        om.RecursiveReset(this->self);
    }
    void FullSync::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->scene);
        om.SetDefaultValue(this->self);
    }
}
namespace Game1_Client{
    void Sync::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->events);
    }
    int Sync::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->events)) return r;
        return 0;
    }
    void Sync::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":81");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Sync::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"events\":", this->events);
#endif
    }
    void Sync::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Game1_Client::Sync*)tar;
        om.Clone_(this->events, out->events);
    }
    int Sync::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->events)) return r;
        return 0;
    }
    void Sync::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->events);
    }
    void Sync::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->events);
    }
}
namespace Client_Game1{
    void Enter::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->loading);
    }
    int Enter::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->loading)) return r;
        return 0;
    }
    void Enter::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":60");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Enter::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"loading\":", this->loading);
#endif
    }
    void Enter::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Client_Game1::Enter*)tar;
        om.Clone_(this->loading, out->loading);
    }
    int Enter::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->loading)) return r;
        return 0;
    }
    void Enter::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->loading);
    }
    void Enter::SetDefaultValue(::xx::ObjManager& om) {
        this->loading = false;
    }
}
namespace Client_Game1{
    void Leave::Write(::xx::ObjManager& om, ::xx::Data& d) const {
    }
    int Leave::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        return 0;
    }
    void Leave::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":61");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Leave::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
#endif
    }
    void Leave::Clone(::xx::ObjManager& om, void* const &tar) const {
    }
    int Leave::RecursiveCheck(::xx::ObjManager& om) const {
        return 0;
    }
    void Leave::RecursiveReset(::xx::ObjManager& om) {
    }
    void Leave::SetDefaultValue(::xx::ObjManager& om) {
    }
}
namespace Client_Game1{
    void SendMessage::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->content);
    }
    int SendMessage::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->content)) return r;
        return 0;
    }
    void SendMessage::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":62");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void SendMessage::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"content\":", this->content);
#endif
    }
    void SendMessage::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Client_Game1::SendMessage*)tar;
        om.Clone_(this->content, out->content);
    }
    int SendMessage::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->content)) return r;
        return 0;
    }
    void SendMessage::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->content);
    }
    void SendMessage::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->content);
    }
}
namespace Game1{
    void Event_PlayerEnter::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        this->BaseType::Write(om, d);
        om.Write(d, this->player);
    }
    int Event_PlayerEnter::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = this->BaseType::Read(om, d)) return r;
        if (int r = om.Read(d, this->player)) return r;
        return 0;
    }
    void Event_PlayerEnter::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":71");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Event_PlayerEnter::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        this->BaseType::AppendCore(om, s);
        om.Append(s, ",\"player\":", this->player);
#endif
    }
    void Event_PlayerEnter::Clone(::xx::ObjManager& om, void* const &tar) const {
        this->BaseType::Clone(om, tar);
        auto out = (::Game1::Event_PlayerEnter*)tar;
        om.Clone_(this->player, out->player);
    }
    int Event_PlayerEnter::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = this->BaseType::RecursiveCheck(om)) return r;
        if (int r = om.RecursiveCheck(this->player)) return r;
        return 0;
    }
    void Event_PlayerEnter::RecursiveReset(::xx::ObjManager& om) {
        this->BaseType::RecursiveReset(om);
        om.RecursiveReset(this->player);
    }
    void Event_PlayerEnter::SetDefaultValue(::xx::ObjManager& om) {
        this->BaseType::SetDefaultValue(om);
        om.SetDefaultValue(this->player);
    }
}
namespace Game1{
    void Event_PlayerLeave::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        this->BaseType::Write(om, d);
        om.Write(d, this->accountId);
    }
    int Event_PlayerLeave::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = this->BaseType::Read(om, d)) return r;
        if (int r = om.Read(d, this->accountId)) return r;
        return 0;
    }
    void Event_PlayerLeave::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":72");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Event_PlayerLeave::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        this->BaseType::AppendCore(om, s);
        om.Append(s, ",\"accountId\":", this->accountId);
#endif
    }
    void Event_PlayerLeave::Clone(::xx::ObjManager& om, void* const &tar) const {
        this->BaseType::Clone(om, tar);
        auto out = (::Game1::Event_PlayerLeave*)tar;
        om.Clone_(this->accountId, out->accountId);
    }
    int Event_PlayerLeave::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = this->BaseType::RecursiveCheck(om)) return r;
        if (int r = om.RecursiveCheck(this->accountId)) return r;
        return 0;
    }
    void Event_PlayerLeave::RecursiveReset(::xx::ObjManager& om) {
        this->BaseType::RecursiveReset(om);
        om.RecursiveReset(this->accountId);
    }
    void Event_PlayerLeave::SetDefaultValue(::xx::ObjManager& om) {
        this->BaseType::SetDefaultValue(om);
        this->accountId = 0;
    }
}
namespace Game1{
    void Event_Message::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        this->BaseType::Write(om, d);
        om.Write(d, this->senderId);
        om.Write(d, this->content);
    }
    int Event_Message::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = this->BaseType::Read(om, d)) return r;
        if (int r = om.Read(d, this->senderId)) return r;
        if (int r = om.Read(d, this->content)) return r;
        return 0;
    }
    void Event_Message::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":73");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Event_Message::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        this->BaseType::AppendCore(om, s);
        om.Append(s, ",\"senderId\":", this->senderId);
        om.Append(s, ",\"content\":", this->content);
#endif
    }
    void Event_Message::Clone(::xx::ObjManager& om, void* const &tar) const {
        this->BaseType::Clone(om, tar);
        auto out = (::Game1::Event_Message*)tar;
        om.Clone_(this->senderId, out->senderId);
        om.Clone_(this->content, out->content);
    }
    int Event_Message::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = this->BaseType::RecursiveCheck(om)) return r;
        if (int r = om.RecursiveCheck(this->senderId)) return r;
        if (int r = om.RecursiveCheck(this->content)) return r;
        return 0;
    }
    void Event_Message::RecursiveReset(::xx::ObjManager& om) {
        this->BaseType::RecursiveReset(om);
        om.RecursiveReset(this->senderId);
        om.RecursiveReset(this->content);
    }
    void Event_Message::SetDefaultValue(::xx::ObjManager& om) {
        this->BaseType::SetDefaultValue(om);
        this->senderId = 0;
        om.SetDefaultValue(this->content);
    }
}

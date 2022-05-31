#include <pkg.h>
#include <pkg.cpp.inc>
void CodeGen_pkg::Register() {
	::xx::ObjManager::Register<::Generic::PlayerInfo>();
	::xx::ObjManager::Register<::Generic::Success>();
	::xx::ObjManager::Register<::Generic::Register>();
	::xx::ObjManager::Register<::All_Db::GetPlayerInfo>();
	::xx::ObjManager::Register<::All_Db::GetPlayerId>();
	::xx::ObjManager::Register<::Lobby_Game1::PlayerEnter>();
	::xx::ObjManager::Register<::Game1_Lobby::PlayerLeave>();
	::xx::ObjManager::Register<::Client_Lobby::EnterGame>();
	::xx::ObjManager::Register<::Client_Lobby::Login>();
	::xx::ObjManager::Register<::Client_Game1::PlayerLeave>();
	::xx::ObjManager::Register<::Client_Game1::AddGold>();
	::xx::ObjManager::Register<::Client_Game1::Enter>();
	::xx::ObjManager::Register<::Client_Game1::Loading>();
	::xx::ObjManager::Register<::Game1_Client::Scene>();
	::xx::ObjManager::Register<::Lobby_Client::Scene>();
	::xx::ObjManager::Register<::Pong>();
	::xx::ObjManager::Register<::Generic::Error>();
	::xx::ObjManager::Register<::Ping>();
}
namespace xx {
	void ObjFuncs<::Game1::PlayerInfo, void>::Write(::xx::ObjManager& om, ::xx::Data& d, ::Game1::PlayerInfo const& in) {
        d.Write(in.playerId);
        d.Write(in.nickname);
        d.Write(in.gold);
    }
	void ObjFuncs<::Game1::PlayerInfo, void>::WriteFast(::xx::ObjManager& om, ::xx::Data& d, ::Game1::PlayerInfo const& in) {
        d.Write<false>(in.playerId);
        d.Write<false>(in.nickname);
        d.Write<false>(in.gold);
    }
	int ObjFuncs<::Game1::PlayerInfo, void>::Read(::xx::ObjManager& om, ::xx::Data_r& d, ::Game1::PlayerInfo& out) {
        if (int r = d.Read(out.playerId)) return r;
        if (int r = d.Read(out.nickname)) return r;
        if (int r = d.Read(out.gold)) return r;
        return 0;
    }
	void ObjFuncs<::Game1::PlayerInfo, void>::Append(ObjManager &om, std::string& s, ::Game1::PlayerInfo const& in) {
#ifndef XX_DISABLE_APPEND
        s.push_back('{');
        AppendCore(om, s, in);
        s.push_back('}');
#endif
    }
	void ObjFuncs<::Game1::PlayerInfo, void>::AppendCore(ObjManager &om, std::string& s, ::Game1::PlayerInfo const& in) {
#ifndef XX_DISABLE_APPEND
        om.Append(s, "\"playerId\":", in.playerId); 
        om.Append(s, ",\"nickname\":", in.nickname);
        om.Append(s, ",\"gold\":", in.gold);
#endif
    }
    void ObjFuncs<::Game1::PlayerInfo>::Clone(::xx::ObjManager& om, ::Game1::PlayerInfo const& in, ::Game1::PlayerInfo &out) {
        om.Clone_(in.playerId, out.playerId);
        om.Clone_(in.nickname, out.nickname);
        om.Clone_(in.gold, out.gold);
    }
    int ObjFuncs<::Game1::PlayerInfo>::RecursiveCheck(::xx::ObjManager& om, ::Game1::PlayerInfo const& in) {
        if (int r = om.RecursiveCheck(in.playerId)) return r;
        if (int r = om.RecursiveCheck(in.nickname)) return r;
        if (int r = om.RecursiveCheck(in.gold)) return r;
        return 0;
    }
    void ObjFuncs<::Game1::PlayerInfo>::RecursiveReset(::xx::ObjManager& om, ::Game1::PlayerInfo& in) {
        om.RecursiveReset(in.playerId);
        om.RecursiveReset(in.nickname);
        om.RecursiveReset(in.gold);
    }
    void ObjFuncs<::Game1::PlayerInfo>::SetDefaultValue(::xx::ObjManager& om, ::Game1::PlayerInfo& in) {
        in.playerId = 0;
        om.SetDefaultValue(in.nickname);
        in.gold = 0;
    }
}
namespace Generic{
    void PlayerInfo::WriteTo(xx::Data& d, int64_t const& id, std::string_view const& username, std::string_view const& password, std::string_view const& nickname, int64_t const& gold) {
        d.Write(xx::TypeId_v<PlayerInfo>);
        d.Write(id);
        d.Write(username);
        d.Write(password);
        d.Write(nickname);
        d.Write(gold);
    }
    void PlayerInfo::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->id);
        d.Write(this->username);
        d.Write(this->password);
        d.Write(this->nickname);
        d.Write(this->gold);
    }
    int PlayerInfo::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->id)) return r;
        if (int r = d.Read(this->username)) return r;
        if (int r = d.Read(this->password)) return r;
        if (int r = d.Read(this->nickname)) return r;
        if (int r = d.Read(this->gold)) return r;
        return 0;
    }
    void PlayerInfo::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":14");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void PlayerInfo::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"id\":", this->id);
        om.Append(s, ",\"username\":", this->username);
        om.Append(s, ",\"password\":", this->password);
        om.Append(s, ",\"nickname\":", this->nickname);
        om.Append(s, ",\"gold\":", this->gold);
#endif
    }
    void PlayerInfo::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Generic::PlayerInfo*)tar;
        om.Clone_(this->id, out->id);
        om.Clone_(this->username, out->username);
        om.Clone_(this->password, out->password);
        om.Clone_(this->nickname, out->nickname);
        om.Clone_(this->gold, out->gold);
    }
    int PlayerInfo::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->id)) return r;
        if (int r = om.RecursiveCheck(this->username)) return r;
        if (int r = om.RecursiveCheck(this->password)) return r;
        if (int r = om.RecursiveCheck(this->nickname)) return r;
        if (int r = om.RecursiveCheck(this->gold)) return r;
        return 0;
    }
    void PlayerInfo::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->id);
        om.RecursiveReset(this->username);
        om.RecursiveReset(this->password);
        om.RecursiveReset(this->nickname);
        om.RecursiveReset(this->gold);
    }
    void PlayerInfo::SetDefaultValue(::xx::ObjManager& om) {
        this->id = 0;
        om.SetDefaultValue(this->username);
        om.SetDefaultValue(this->password);
        om.SetDefaultValue(this->nickname);
        this->gold = 0;
    }
}
namespace Generic{
    void Success::WriteTo(xx::Data& d, int64_t const& value) {
        d.Write(xx::TypeId_v<Success>);
        d.Write(value);
    }
    void Success::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->value);
    }
    int Success::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->value)) return r;
        return 0;
    }
    void Success::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":12");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Success::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"value\":", this->value);
#endif
    }
    void Success::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Generic::Success*)tar;
        om.Clone_(this->value, out->value);
    }
    int Success::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->value)) return r;
        return 0;
    }
    void Success::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->value);
    }
    void Success::SetDefaultValue(::xx::ObjManager& om) {
        this->value = 0;
    }
}
namespace Generic{
    void Register::WriteTo(xx::Data& d, uint32_t const& id) {
        d.Write(xx::TypeId_v<Register>);
        d.Write(id);
    }
    void Register::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->id);
    }
    int Register::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->id)) return r;
        return 0;
    }
    void Register::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":11");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Register::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"id\":", this->id);
#endif
    }
    void Register::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Generic::Register*)tar;
        om.Clone_(this->id, out->id);
    }
    int Register::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->id)) return r;
        return 0;
    }
    void Register::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->id);
    }
    void Register::SetDefaultValue(::xx::ObjManager& om) {
        this->id = 0;
    }
}
namespace All_Db{
    void GetPlayerInfo::WriteTo(xx::Data& d, int64_t const& id) {
        d.Write(xx::TypeId_v<GetPlayerInfo>);
        d.Write(id);
    }
    void GetPlayerInfo::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->id);
    }
    int GetPlayerInfo::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->id)) return r;
        return 0;
    }
    void GetPlayerInfo::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":202");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void GetPlayerInfo::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"id\":", this->id);
#endif
    }
    void GetPlayerInfo::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::All_Db::GetPlayerInfo*)tar;
        om.Clone_(this->id, out->id);
    }
    int GetPlayerInfo::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->id)) return r;
        return 0;
    }
    void GetPlayerInfo::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->id);
    }
    void GetPlayerInfo::SetDefaultValue(::xx::ObjManager& om) {
        this->id = 0;
    }
}
namespace All_Db{
    void GetPlayerId::WriteTo(xx::Data& d, std::string_view const& username, std::string_view const& password) {
        d.Write(xx::TypeId_v<GetPlayerId>);
        d.Write(username);
        d.Write(password);
    }
    void GetPlayerId::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->username);
        d.Write(this->password);
    }
    int GetPlayerId::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->username)) return r;
        if (int r = d.Read(this->password)) return r;
        return 0;
    }
    void GetPlayerId::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":201");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void GetPlayerId::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"username\":", this->username);
        om.Append(s, ",\"password\":", this->password);
#endif
    }
    void GetPlayerId::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::All_Db::GetPlayerId*)tar;
        om.Clone_(this->username, out->username);
        om.Clone_(this->password, out->password);
    }
    int GetPlayerId::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->username)) return r;
        if (int r = om.RecursiveCheck(this->password)) return r;
        return 0;
    }
    void GetPlayerId::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->username);
        om.RecursiveReset(this->password);
    }
    void GetPlayerId::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->username);
        om.SetDefaultValue(this->password);
    }
}
namespace Lobby_Game1{
    void PlayerEnter::WriteTo(xx::Data& d, uint32_t const& gatewayId, uint32_t const& clientId, int64_t const& playerId) {
        d.Write(xx::TypeId_v<PlayerEnter>);
        d.Write(gatewayId);
        d.Write(clientId);
        d.Write(playerId);
    }
    void PlayerEnter::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->gatewayId);
        d.Write(this->clientId);
        d.Write(this->playerId);
    }
    int PlayerEnter::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->gatewayId)) return r;
        if (int r = d.Read(this->clientId)) return r;
        if (int r = d.Read(this->playerId)) return r;
        return 0;
    }
    void PlayerEnter::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":301");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void PlayerEnter::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"gatewayId\":", this->gatewayId);
        om.Append(s, ",\"clientId\":", this->clientId);
        om.Append(s, ",\"playerId\":", this->playerId);
#endif
    }
    void PlayerEnter::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Lobby_Game1::PlayerEnter*)tar;
        om.Clone_(this->gatewayId, out->gatewayId);
        om.Clone_(this->clientId, out->clientId);
        om.Clone_(this->playerId, out->playerId);
    }
    int PlayerEnter::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->gatewayId)) return r;
        if (int r = om.RecursiveCheck(this->clientId)) return r;
        if (int r = om.RecursiveCheck(this->playerId)) return r;
        return 0;
    }
    void PlayerEnter::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->gatewayId);
        om.RecursiveReset(this->clientId);
        om.RecursiveReset(this->playerId);
    }
    void PlayerEnter::SetDefaultValue(::xx::ObjManager& om) {
        this->gatewayId = 0;
        this->clientId = 0;
        this->playerId = 0;
    }
}
namespace Game1_Lobby{
    void PlayerLeave::WriteTo(xx::Data& d, int64_t const& playerId) {
        d.Write(xx::TypeId_v<PlayerLeave>);
        d.Write(playerId);
    }
    void PlayerLeave::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->playerId);
    }
    int PlayerLeave::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->playerId)) return r;
        return 0;
    }
    void PlayerLeave::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":401");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void PlayerLeave::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"playerId\":", this->playerId);
#endif
    }
    void PlayerLeave::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Game1_Lobby::PlayerLeave*)tar;
        om.Clone_(this->playerId, out->playerId);
    }
    int PlayerLeave::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->playerId)) return r;
        return 0;
    }
    void PlayerLeave::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->playerId);
    }
    void PlayerLeave::SetDefaultValue(::xx::ObjManager& om) {
        this->playerId = 0;
    }
}
namespace Client_Lobby{
    void EnterGame::WriteTo(xx::Data& d, ::std::vector<int32_t> const& gamePath) {
        d.Write(xx::TypeId_v<EnterGame>);
        d.Write(gamePath);
    }
    void EnterGame::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->gamePath);
    }
    int EnterGame::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->gamePath)) return r;
        return 0;
    }
    void EnterGame::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":502");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void EnterGame::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"gamePath\":", this->gamePath);
#endif
    }
    void EnterGame::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Client_Lobby::EnterGame*)tar;
        om.Clone_(this->gamePath, out->gamePath);
    }
    int EnterGame::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->gamePath)) return r;
        return 0;
    }
    void EnterGame::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->gamePath);
    }
    void EnterGame::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->gamePath);
    }
}
namespace Client_Lobby{
    void Login::WriteTo(xx::Data& d, std::string_view const& username, std::string_view const& password) {
        d.Write(xx::TypeId_v<Login>);
        d.Write(username);
        d.Write(password);
    }
    void Login::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->username);
        d.Write(this->password);
    }
    int Login::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->username)) return r;
        if (int r = d.Read(this->password)) return r;
        return 0;
    }
    void Login::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":501");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Login::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"username\":", this->username);
        om.Append(s, ",\"password\":", this->password);
#endif
    }
    void Login::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Client_Lobby::Login*)tar;
        om.Clone_(this->username, out->username);
        om.Clone_(this->password, out->password);
    }
    int Login::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->username)) return r;
        if (int r = om.RecursiveCheck(this->password)) return r;
        return 0;
    }
    void Login::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->username);
        om.RecursiveReset(this->password);
    }
    void Login::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->username);
        om.SetDefaultValue(this->password);
    }
}
namespace Client_Game1{
    void PlayerLeave::WriteTo(xx::Data& d) {
        d.Write(xx::TypeId_v<PlayerLeave>);
    }
    void PlayerLeave::Write(::xx::ObjManager& om, ::xx::Data& d) const {
    }
    int PlayerLeave::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        return 0;
    }
    void PlayerLeave::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":604");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void PlayerLeave::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
#endif
    }
    void PlayerLeave::Clone(::xx::ObjManager& om, void* const &tar) const {
    }
    int PlayerLeave::RecursiveCheck(::xx::ObjManager& om) const {
        return 0;
    }
    void PlayerLeave::RecursiveReset(::xx::ObjManager& om) {
    }
    void PlayerLeave::SetDefaultValue(::xx::ObjManager& om) {
    }
}
namespace Client_Game1{
    void AddGold::WriteTo(xx::Data& d, int64_t const& value) {
        d.Write(xx::TypeId_v<AddGold>);
        d.Write(value);
    }
    void AddGold::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->value);
    }
    int AddGold::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->value)) return r;
        return 0;
    }
    void AddGold::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":603");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void AddGold::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"value\":", this->value);
#endif
    }
    void AddGold::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Client_Game1::AddGold*)tar;
        om.Clone_(this->value, out->value);
    }
    int AddGold::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->value)) return r;
        return 0;
    }
    void AddGold::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->value);
    }
    void AddGold::SetDefaultValue(::xx::ObjManager& om) {
        this->value = 0;
    }
}
namespace Client_Game1{
    void Enter::WriteTo(xx::Data& d) {
        d.Write(xx::TypeId_v<Enter>);
    }
    void Enter::Write(::xx::ObjManager& om, ::xx::Data& d) const {
    }
    int Enter::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        return 0;
    }
    void Enter::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":602");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Enter::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
#endif
    }
    void Enter::Clone(::xx::ObjManager& om, void* const &tar) const {
    }
    int Enter::RecursiveCheck(::xx::ObjManager& om) const {
        return 0;
    }
    void Enter::RecursiveReset(::xx::ObjManager& om) {
    }
    void Enter::SetDefaultValue(::xx::ObjManager& om) {
    }
}
namespace Client_Game1{
    void Loading::WriteTo(xx::Data& d) {
        d.Write(xx::TypeId_v<Loading>);
    }
    void Loading::Write(::xx::ObjManager& om, ::xx::Data& d) const {
    }
    int Loading::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        return 0;
    }
    void Loading::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":601");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Loading::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
#endif
    }
    void Loading::Clone(::xx::ObjManager& om, void* const &tar) const {
    }
    int Loading::RecursiveCheck(::xx::ObjManager& om) const {
        return 0;
    }
    void Loading::RecursiveReset(::xx::ObjManager& om) {
    }
    void Loading::SetDefaultValue(::xx::ObjManager& om) {
    }
}
namespace Game1_Client{
    void Scene::WriteTo(xx::Data& d, ::std::map<int32_t, ::Game1::PlayerInfo> const& players) {
        d.Write(xx::TypeId_v<Scene>);
        d.Write(players);
    }
    void Scene::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->players);
    }
    int Scene::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->players)) return r;
        return 0;
    }
    void Scene::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":701");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Scene::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"players\":", this->players);
#endif
    }
    void Scene::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Game1_Client::Scene*)tar;
        om.Clone_(this->players, out->players);
    }
    int Scene::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->players)) return r;
        return 0;
    }
    void Scene::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->players);
    }
    void Scene::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->players);
    }
}
namespace Lobby_Client{
    void Scene::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->playerInfo);
        d.Write(this->gamesIntro);
    }
    int Scene::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->playerInfo)) return r;
        if (int r = d.Read(this->gamesIntro)) return r;
        return 0;
    }
    void Scene::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":801");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Scene::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"playerInfo\":", this->playerInfo);
        om.Append(s, ",\"gamesIntro\":", this->gamesIntro);
#endif
    }
    void Scene::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Lobby_Client::Scene*)tar;
        om.Clone_(this->playerInfo, out->playerInfo);
        om.Clone_(this->gamesIntro, out->gamesIntro);
    }
    int Scene::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->playerInfo)) return r;
        if (int r = om.RecursiveCheck(this->gamesIntro)) return r;
        return 0;
    }
    void Scene::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->playerInfo);
        om.RecursiveReset(this->gamesIntro);
    }
    void Scene::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->playerInfo);
        om.SetDefaultValue(this->gamesIntro);
    }
}
void Pong::WriteTo(xx::Data& d, int64_t const& ticks) {
    d.Write(xx::TypeId_v<Pong>);
    d.Write(ticks);
}
void Pong::Write(::xx::ObjManager& om, ::xx::Data& d) const {
    d.Write(this->ticks);
}
int Pong::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
    if (int r = d.Read(this->ticks)) return r;
    return 0;
}
void Pong::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
    ::xx::Append(s, "{\"__typeId__\":1283");
    this->AppendCore(om, s);
    s.push_back('}');
#endif
}
void Pong::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
    om.Append(s, ",\"ticks\":", this->ticks);
#endif
}
void Pong::Clone(::xx::ObjManager& om, void* const &tar) const {
    auto out = (::Pong*)tar;
    om.Clone_(this->ticks, out->ticks);
}
int Pong::RecursiveCheck(::xx::ObjManager& om) const {
    if (int r = om.RecursiveCheck(this->ticks)) return r;
    return 0;
}
void Pong::RecursiveReset(::xx::ObjManager& om) {
    om.RecursiveReset(this->ticks);
}
void Pong::SetDefaultValue(::xx::ObjManager& om) {
    this->ticks = 0;
}
namespace Generic{
    void Error::WriteTo(xx::Data& d, int64_t const& number, std::string_view const& message) {
        d.Write(xx::TypeId_v<Error>);
        d.Write(number);
        d.Write(message);
    }
    void Error::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->number);
        d.Write(this->message);
    }
    int Error::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->number)) return r;
        if (int r = d.Read(this->message)) return r;
        return 0;
    }
    void Error::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":13");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Error::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"number\":", this->number);
        om.Append(s, ",\"message\":", this->message);
#endif
    }
    void Error::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Generic::Error*)tar;
        om.Clone_(this->number, out->number);
        om.Clone_(this->message, out->message);
    }
    int Error::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->number)) return r;
        if (int r = om.RecursiveCheck(this->message)) return r;
        return 0;
    }
    void Error::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->number);
        om.RecursiveReset(this->message);
    }
    void Error::SetDefaultValue(::xx::ObjManager& om) {
        this->number = 0;
        om.SetDefaultValue(this->message);
    }
}
void Ping::WriteTo(xx::Data& d, int64_t const& ticks) {
    d.Write(xx::TypeId_v<Ping>);
    d.Write(ticks);
}
void Ping::Write(::xx::ObjManager& om, ::xx::Data& d) const {
    d.Write(this->ticks);
}
int Ping::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
    if (int r = d.Read(this->ticks)) return r;
    return 0;
}
void Ping::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
    ::xx::Append(s, "{\"__typeId__\":2062");
    this->AppendCore(om, s);
    s.push_back('}');
#endif
}
void Ping::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
    om.Append(s, ",\"ticks\":", this->ticks);
#endif
}
void Ping::Clone(::xx::ObjManager& om, void* const &tar) const {
    auto out = (::Ping*)tar;
    om.Clone_(this->ticks, out->ticks);
}
int Ping::RecursiveCheck(::xx::ObjManager& om) const {
    if (int r = om.RecursiveCheck(this->ticks)) return r;
    return 0;
}
void Ping::RecursiveReset(::xx::ObjManager& om) {
    om.RecursiveReset(this->ticks);
}
void Ping::SetDefaultValue(::xx::ObjManager& om) {
    this->ticks = 0;
}

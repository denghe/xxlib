#include <pkg.h>
#include <pkg.cpp.inc>
void CodeGen_pkg::Register() {
	::xx::ObjManager::Register<::Ping>();
	::xx::ObjManager::Register<::Pong>();
	::xx::ObjManager::Register<::Game1_Lobby::PlayerLeave>();
	::xx::ObjManager::Register<::Lobby_Game1::PlayerEnter>();
	::xx::ObjManager::Register<::All_Db::GetPlayerId>();
	::xx::ObjManager::Register<::All_Db::GetPlayerInfo>();
	::xx::ObjManager::Register<::Generic::Success>();
	::xx::ObjManager::Register<::Generic::Error>();
	::xx::ObjManager::Register<::Generic::PlayerInfo>();
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
        ::xx::Append(s, "{\"__typeId__\":10");
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
        ::xx::Append(s, "{\"__typeId__\":11");
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
        ::xx::Append(s, "{\"__typeId__\":12");
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

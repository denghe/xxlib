#include <ss.h>
#include <ss.cpp.inc>
void CodeGen_ss::Register() {
	::xx::ObjManager::Register<::SS::Scene>();
	::xx::ObjManager::Register<::SS::Shooter>();
	::xx::ObjManager::Register<::SS::Bullet>();
	::xx::ObjManager::Register<::SS_S2C::EnterResult>();
	::xx::ObjManager::Register<::SS_S2C::Sync>();
	::xx::ObjManager::Register<::SS_S2C::Event>();
	::xx::ObjManager::Register<::SS_C2S::Enter>();
	::xx::ObjManager::Register<::SS_C2S::Cmd>();
	::xx::ObjManager::Register<::SS::Bullet_Straight>();
	::xx::ObjManager::Register<::SS::Bullet_Track>();
}
namespace xx {
	void ObjFuncs<::SS::XY, void>::Write(::xx::ObjManager& om, ::xx::Data& d, ::SS::XY const& in) {
        d.Write(in.x);
        d.Write(in.y);
    }
	void ObjFuncs<::SS::XY, void>::WriteFast(::xx::ObjManager& om, ::xx::Data& d, ::SS::XY const& in) {
        d.Write<false>(in.x);
        d.Write<false>(in.y);
    }
	int ObjFuncs<::SS::XY, void>::Read(::xx::ObjManager& om, ::xx::Data_r& d, ::SS::XY& out) {
        if (int r = d.Read(out.x)) return r;
        if (int r = d.Read(out.y)) return r;
        return 0;
    }
	void ObjFuncs<::SS::XY, void>::Append(ObjManager &om, std::string& s, ::SS::XY const& in) {
#ifndef XX_DISABLE_APPEND
        s.push_back('{');
        AppendCore(om, s, in);
        s.push_back('}');
#endif
    }
	void ObjFuncs<::SS::XY, void>::AppendCore(ObjManager &om, std::string& s, ::SS::XY const& in) {
#ifndef XX_DISABLE_APPEND
        om.Append(s, "\"x\":", in.x); 
        om.Append(s, ",\"y\":", in.y);
#endif
    }
    void ObjFuncs<::SS::XY>::Clone(::xx::ObjManager& om, ::SS::XY const& in, ::SS::XY &out) {
        om.Clone_(in.x, out.x);
        om.Clone_(in.y, out.y);
    }
    int ObjFuncs<::SS::XY>::RecursiveCheck(::xx::ObjManager& om, ::SS::XY const& in) {
        if (int r = om.RecursiveCheck(in.x)) return r;
        if (int r = om.RecursiveCheck(in.y)) return r;
        return 0;
    }
    void ObjFuncs<::SS::XY>::RecursiveReset(::xx::ObjManager& om, ::SS::XY& in) {
        om.RecursiveReset(in.x);
        om.RecursiveReset(in.y);
    }
    void ObjFuncs<::SS::XY>::SetDefaultValue(::xx::ObjManager& om, ::SS::XY& in) {
        in.x = 0;
        in.y = 0;
    }
	void ObjFuncs<::SS::ControlState, void>::Write(::xx::ObjManager& om, ::xx::Data& d, ::SS::ControlState const& in) {
        d.Write(in.aimPos);
        d.Write(in.moveLeft);
        d.Write(in.moveRight);
        d.Write(in.moveUp);
        d.Write(in.moveDown);
        d.Write(in.button1);
        d.Write(in.button2);
    }
	void ObjFuncs<::SS::ControlState, void>::WriteFast(::xx::ObjManager& om, ::xx::Data& d, ::SS::ControlState const& in) {
        d.Write<false>(in.aimPos);
        d.Write<false>(in.moveLeft);
        d.Write<false>(in.moveRight);
        d.Write<false>(in.moveUp);
        d.Write<false>(in.moveDown);
        d.Write<false>(in.button1);
        d.Write<false>(in.button2);
    }
	int ObjFuncs<::SS::ControlState, void>::Read(::xx::ObjManager& om, ::xx::Data_r& d, ::SS::ControlState& out) {
        if (int r = d.Read(out.aimPos)) return r;
        if (int r = d.Read(out.moveLeft)) return r;
        if (int r = d.Read(out.moveRight)) return r;
        if (int r = d.Read(out.moveUp)) return r;
        if (int r = d.Read(out.moveDown)) return r;
        if (int r = d.Read(out.button1)) return r;
        if (int r = d.Read(out.button2)) return r;
        return 0;
    }
	void ObjFuncs<::SS::ControlState, void>::Append(ObjManager &om, std::string& s, ::SS::ControlState const& in) {
#ifndef XX_DISABLE_APPEND
        s.push_back('{');
        AppendCore(om, s, in);
        s.push_back('}');
#endif
    }
	void ObjFuncs<::SS::ControlState, void>::AppendCore(ObjManager &om, std::string& s, ::SS::ControlState const& in) {
#ifndef XX_DISABLE_APPEND
        om.Append(s, "\"aimPos\":", in.aimPos); 
        om.Append(s, ",\"moveLeft\":", in.moveLeft);
        om.Append(s, ",\"moveRight\":", in.moveRight);
        om.Append(s, ",\"moveUp\":", in.moveUp);
        om.Append(s, ",\"moveDown\":", in.moveDown);
        om.Append(s, ",\"button1\":", in.button1);
        om.Append(s, ",\"button2\":", in.button2);
#endif
    }
    void ObjFuncs<::SS::ControlState>::Clone(::xx::ObjManager& om, ::SS::ControlState const& in, ::SS::ControlState &out) {
        om.Clone_(in.aimPos, out.aimPos);
        om.Clone_(in.moveLeft, out.moveLeft);
        om.Clone_(in.moveRight, out.moveRight);
        om.Clone_(in.moveUp, out.moveUp);
        om.Clone_(in.moveDown, out.moveDown);
        om.Clone_(in.button1, out.button1);
        om.Clone_(in.button2, out.button2);
    }
    int ObjFuncs<::SS::ControlState>::RecursiveCheck(::xx::ObjManager& om, ::SS::ControlState const& in) {
        if (int r = om.RecursiveCheck(in.aimPos)) return r;
        if (int r = om.RecursiveCheck(in.moveLeft)) return r;
        if (int r = om.RecursiveCheck(in.moveRight)) return r;
        if (int r = om.RecursiveCheck(in.moveUp)) return r;
        if (int r = om.RecursiveCheck(in.moveDown)) return r;
        if (int r = om.RecursiveCheck(in.button1)) return r;
        if (int r = om.RecursiveCheck(in.button2)) return r;
        return 0;
    }
    void ObjFuncs<::SS::ControlState>::RecursiveReset(::xx::ObjManager& om, ::SS::ControlState& in) {
        om.RecursiveReset(in.aimPos);
        om.RecursiveReset(in.moveLeft);
        om.RecursiveReset(in.moveRight);
        om.RecursiveReset(in.moveUp);
        om.RecursiveReset(in.moveDown);
        om.RecursiveReset(in.button1);
        om.RecursiveReset(in.button2);
    }
    void ObjFuncs<::SS::ControlState>::SetDefaultValue(::xx::ObjManager& om, ::SS::ControlState& in) {
        om.SetDefaultValue(in.aimPos);
        in.moveLeft = false;
        in.moveRight = false;
        in.moveUp = false;
        in.moveDown = false;
        in.button1 = false;
        in.button2 = false;
    }
}
namespace SS{
    void Scene::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->frameNumber);
        om.Write(d, this->shooters);
    }
    int Scene::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->frameNumber)) return r;
        if (int r = om.Read(d, this->shooters)) return r;
        return 0;
    }
    void Scene::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":10");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Scene::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"frameNumber\":", this->frameNumber);
        om.Append(s, ",\"shooters\":", this->shooters);
#endif
    }
    void Scene::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::SS::Scene*)tar;
        om.Clone_(this->frameNumber, out->frameNumber);
        om.Clone_(this->shooters, out->shooters);
    }
    int Scene::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->frameNumber)) return r;
        if (int r = om.RecursiveCheck(this->shooters)) return r;
        return 0;
    }
    void Scene::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->frameNumber);
        om.RecursiveReset(this->shooters);
    }
    void Scene::SetDefaultValue(::xx::ObjManager& om) {
        this->frameNumber = 0;
        om.SetDefaultValue(this->shooters);
    }
}
namespace SS{
    void Shooter::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->scene);
        d.Write(this->clientId);
        d.Write(this->bodyAngle);
        d.Write(this->speed);
        d.Write(this->pos);
        om.Write(d, this->bullets);
        d.Write(this->cs);
    }
    int Shooter::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->scene)) return r;
        if (int r = d.Read(this->clientId)) return r;
        if (int r = d.Read(this->bodyAngle)) return r;
        if (int r = d.Read(this->speed)) return r;
        if (int r = d.Read(this->pos)) return r;
        if (int r = om.Read(d, this->bullets)) return r;
        if (int r = d.Read(this->cs)) return r;
        return 0;
    }
    void Shooter::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":20");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Shooter::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"scene\":", this->scene);
        om.Append(s, ",\"clientId\":", this->clientId);
        om.Append(s, ",\"bodyAngle\":", this->bodyAngle);
        om.Append(s, ",\"speed\":", this->speed);
        om.Append(s, ",\"pos\":", this->pos);
        om.Append(s, ",\"bullets\":", this->bullets);
        om.Append(s, ",\"cs\":", this->cs);
#endif
    }
    void Shooter::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::SS::Shooter*)tar;
        om.Clone_(this->scene, out->scene);
        om.Clone_(this->clientId, out->clientId);
        om.Clone_(this->bodyAngle, out->bodyAngle);
        om.Clone_(this->speed, out->speed);
        om.Clone_(this->pos, out->pos);
        om.Clone_(this->bullets, out->bullets);
        om.Clone_(this->cs, out->cs);
    }
    int Shooter::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->scene)) return r;
        if (int r = om.RecursiveCheck(this->clientId)) return r;
        if (int r = om.RecursiveCheck(this->bodyAngle)) return r;
        if (int r = om.RecursiveCheck(this->speed)) return r;
        if (int r = om.RecursiveCheck(this->pos)) return r;
        if (int r = om.RecursiveCheck(this->bullets)) return r;
        if (int r = om.RecursiveCheck(this->cs)) return r;
        return 0;
    }
    void Shooter::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->scene);
        om.RecursiveReset(this->clientId);
        om.RecursiveReset(this->bodyAngle);
        om.RecursiveReset(this->speed);
        om.RecursiveReset(this->pos);
        om.RecursiveReset(this->bullets);
        om.RecursiveReset(this->cs);
    }
    void Shooter::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->scene);
        this->clientId = 0;
        this->bodyAngle = 0;
        this->speed = 10;
        om.SetDefaultValue(this->pos);
        om.SetDefaultValue(this->bullets);
        om.SetDefaultValue(this->cs);
    }
}
namespace SS{
    void Bullet::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->shooter);
        d.Write(this->speed);
        d.Write(this->life);
        d.Write(this->pos);
    }
    int Bullet::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->shooter)) return r;
        if (int r = d.Read(this->speed)) return r;
        if (int r = d.Read(this->life)) return r;
        if (int r = d.Read(this->pos)) return r;
        return 0;
    }
    void Bullet::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":30");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Bullet::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"shooter\":", this->shooter);
        om.Append(s, ",\"speed\":", this->speed);
        om.Append(s, ",\"life\":", this->life);
        om.Append(s, ",\"pos\":", this->pos);
#endif
    }
    void Bullet::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::SS::Bullet*)tar;
        om.Clone_(this->shooter, out->shooter);
        om.Clone_(this->speed, out->speed);
        om.Clone_(this->life, out->life);
        om.Clone_(this->pos, out->pos);
    }
    int Bullet::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->shooter)) return r;
        if (int r = om.RecursiveCheck(this->speed)) return r;
        if (int r = om.RecursiveCheck(this->life)) return r;
        if (int r = om.RecursiveCheck(this->pos)) return r;
        return 0;
    }
    void Bullet::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->shooter);
        om.RecursiveReset(this->speed);
        om.RecursiveReset(this->life);
        om.RecursiveReset(this->pos);
    }
    void Bullet::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->shooter);
        this->speed = 60;
        this->life = 0;
        om.SetDefaultValue(this->pos);
    }
}
namespace SS_S2C{
    void EnterResult::WriteTo(xx::Data& d, uint32_t const& clientId) {
        d.Write(xx::TypeId_v<EnterResult>);
        d.Write(clientId);
    }
    void EnterResult::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->clientId);
    }
    int EnterResult::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->clientId)) return r;
        return 0;
    }
    void EnterResult::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":50");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void EnterResult::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"clientId\":", this->clientId);
#endif
    }
    void EnterResult::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::SS_S2C::EnterResult*)tar;
        om.Clone_(this->clientId, out->clientId);
    }
    int EnterResult::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->clientId)) return r;
        return 0;
    }
    void EnterResult::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->clientId);
    }
    void EnterResult::SetDefaultValue(::xx::ObjManager& om) {
        this->clientId = 0;
    }
}
namespace SS_S2C{
    void Sync::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->scene);
    }
    int Sync::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->scene)) return r;
        return 0;
    }
    void Sync::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":51");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Sync::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"scene\":", this->scene);
#endif
    }
    void Sync::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::SS_S2C::Sync*)tar;
        om.Clone_(this->scene, out->scene);
    }
    int Sync::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->scene)) return r;
        return 0;
    }
    void Sync::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->scene);
    }
    void Sync::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->scene);
    }
}
namespace SS_S2C{
    void Event::WriteTo(xx::Data& d, int32_t const& frameNumber, ::std::vector<uint32_t> const& joins, ::std::vector<::std::tuple<uint32_t, ::SS::ControlState>> const& css) {
        d.Write(xx::TypeId_v<Event>);
        d.Write(frameNumber);
        d.Write(joins);
        d.Write(css);
    }
    void Event::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->frameNumber);
        d.Write(this->joins);
        d.Write(this->css);
    }
    int Event::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->frameNumber)) return r;
        if (int r = d.Read(this->joins)) return r;
        if (int r = d.Read(this->css)) return r;
        return 0;
    }
    void Event::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":52");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Event::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"frameNumber\":", this->frameNumber);
        om.Append(s, ",\"joins\":", this->joins);
        om.Append(s, ",\"css\":", this->css);
#endif
    }
    void Event::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::SS_S2C::Event*)tar;
        om.Clone_(this->frameNumber, out->frameNumber);
        om.Clone_(this->joins, out->joins);
        om.Clone_(this->css, out->css);
    }
    int Event::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->frameNumber)) return r;
        if (int r = om.RecursiveCheck(this->joins)) return r;
        if (int r = om.RecursiveCheck(this->css)) return r;
        return 0;
    }
    void Event::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->frameNumber);
        om.RecursiveReset(this->joins);
        om.RecursiveReset(this->css);
    }
    void Event::SetDefaultValue(::xx::ObjManager& om) {
        this->frameNumber = 0;
        om.SetDefaultValue(this->joins);
        om.SetDefaultValue(this->css);
    }
}
namespace SS_C2S{
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
        ::xx::Append(s, "{\"__typeId__\":40");
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
namespace SS_C2S{
    void Cmd::WriteTo(xx::Data& d, ::SS::ControlState const& cs) {
        d.Write(xx::TypeId_v<Cmd>);
        d.Write(cs);
    }
    void Cmd::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->cs);
    }
    int Cmd::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->cs)) return r;
        return 0;
    }
    void Cmd::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":41");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Cmd::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"cs\":", this->cs);
#endif
    }
    void Cmd::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::SS_C2S::Cmd*)tar;
        om.Clone_(this->cs, out->cs);
    }
    int Cmd::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->cs)) return r;
        return 0;
    }
    void Cmd::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->cs);
    }
    void Cmd::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->cs);
    }
}
namespace SS{
    void Bullet_Straight::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        this->BaseType::Write(om, d);
        d.Write(this->inc);
    }
    int Bullet_Straight::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = this->BaseType::Read(om, d)) return r;
        if (int r = d.Read(this->inc)) return r;
        return 0;
    }
    void Bullet_Straight::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":31");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Bullet_Straight::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        this->BaseType::AppendCore(om, s);
        om.Append(s, ",\"inc\":", this->inc);
#endif
    }
    void Bullet_Straight::Clone(::xx::ObjManager& om, void* const &tar) const {
        this->BaseType::Clone(om, tar);
        auto out = (::SS::Bullet_Straight*)tar;
        om.Clone_(this->inc, out->inc);
    }
    int Bullet_Straight::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = this->BaseType::RecursiveCheck(om)) return r;
        if (int r = om.RecursiveCheck(this->inc)) return r;
        return 0;
    }
    void Bullet_Straight::RecursiveReset(::xx::ObjManager& om) {
        this->BaseType::RecursiveReset(om);
        om.RecursiveReset(this->inc);
    }
    void Bullet_Straight::SetDefaultValue(::xx::ObjManager& om) {
        this->BaseType::SetDefaultValue(om);
        om.SetDefaultValue(this->inc);
    }
}
namespace SS{
    void Bullet_Track::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        this->BaseType::Write(om, d);
        om.Write(d, this->target);
    }
    int Bullet_Track::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = this->BaseType::Read(om, d)) return r;
        if (int r = om.Read(d, this->target)) return r;
        return 0;
    }
    void Bullet_Track::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":32");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Bullet_Track::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        this->BaseType::AppendCore(om, s);
        om.Append(s, ",\"target\":", this->target);
#endif
    }
    void Bullet_Track::Clone(::xx::ObjManager& om, void* const &tar) const {
        this->BaseType::Clone(om, tar);
        auto out = (::SS::Bullet_Track*)tar;
        om.Clone_(this->target, out->target);
    }
    int Bullet_Track::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = this->BaseType::RecursiveCheck(om)) return r;
        if (int r = om.RecursiveCheck(this->target)) return r;
        return 0;
    }
    void Bullet_Track::RecursiveReset(::xx::ObjManager& om) {
        this->BaseType::RecursiveReset(om);
        om.RecursiveReset(this->target);
    }
    void Bullet_Track::SetDefaultValue(::xx::ObjManager& om) {
        this->BaseType::SetDefaultValue(om);
        om.SetDefaultValue(this->target);
    }
}

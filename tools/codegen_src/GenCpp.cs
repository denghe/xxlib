using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Collections.Generic;

// 基于 xx_obj.h
// 针对 Shared<T>, 生成 xx::Shared<T>. 要求每个用于智能指针的类 标注 [TypeId]. 检查到漏标就报错
// 针对 Weak<T> 类成员属性, 生成 xx::Weak<T>
// 针对 struct 或标记有 [Struct] 的 class: 生成 ObjFuncs 模板特化适配

public static class GenCpp {
    // 简化传参
    static Cfg cfg;
    static List<string> createEmptyFiles = new List<string>();

    public static void Gen() {
        cfg = TypeHelpers.cfg;
        createEmptyFiles.Clear();
        cfg.types._SortByInheritRelation();
        cfg.classsStructs._SortByInheritRelation();

        Gen_h();
        Gen_cpp();
        Gen_ajson_h();
        Gen_empties();
    }

    public static void Gen_h() {
        var sb = new StringBuilder();

        // 校验和注册

        sb.Append(@"#pragma once
#include ""xx_obj.h""
#include """ + cfg.name + @".h.inc""
namespace " + cfg.name + @" {
	struct CodeGenMd5 {
		inline static const ::std::string value = """ + StringHelpers.MD5PlaceHolder + @""";
    };
	struct CodeGenTypes {
        static void Register();
    };
");

        // 前置切片文件

        createEmptyFiles.Add(cfg.name + ".h.inc");

        // 所有 class 的预声明( 

        for (int i = 0; i < cfg.localClasss.Count; ++i) {
            var c = cfg.localClasss[i];
            var o = c._GetInstance();

            if (c.Namespace != null && (i == 0 || (i > 0 && cfg.localClasss[i - 1].Namespace != c.Namespace))) // namespace 去重
            {
                sb.Append(@"
namespace " + c.Namespace.Replace(".", "::") + @" {");
            }

            sb.Append(@"
    struct " + c.Name + ";");

            if (c.Namespace != null && ((i < cfg.classs.Count - 1 && cfg.classs[i + 1].Namespace != c.Namespace) || i == cfg.classs.Count - 1)) {
                sb.Append(@"
}");
            }
        }

        sb.Append(@"
}
namespace xx {");
        foreach (var c in cfg.classs) {
            if (c._IsStruct()) return;
            var ctn = c._GetTypeDecl_Cpp();
            var typeId = c._GetTypeId();
            if (typeId.HasValue) {
                sb.Append(@"
    template<> struct TypeId<" + ctn + @"> { static const uint16_t value = " + typeId + @"; };");
            }
        }
        sb.Append(@"
}
");

        for (int i = 0; i < cfg.enums.Count; ++i) {
            var e = cfg.enums[i];
            if (e.Namespace != null && (i == 0 || (i > 0 && cfg.enums[i - 1].Namespace != e.Namespace))) // namespace 去重
            {
                sb.Append(@"
namespace " + e.Namespace.Replace(".", "::") + @" {");
            }

            sb.Append(e._GetDesc()._GetComment_Cpp(4) + @"
    enum class " + e.Name + @" : " + e._GetEnumUnderlyingTypeName_Cpp() + @" {");

            var fs = e._GetEnumFields();
            foreach (var f in fs) {
                sb.Append(f._GetDesc()._GetComment_Cpp(8) + @"
        " + f.Name + " = " + f._GetEnumValue(e) + ",");
            }

            sb.Append(@"
    };");

            if (e.Namespace != null && ((i < cfg.enums.Count - 1 && cfg.enums[i + 1].Namespace != e.Namespace) || i == cfg.enums.Count - 1)) {
                sb.Append(@"
}");
            }
        }

        for (int i = 0; i < cfg.classs.Count; ++i) {
            var c = cfg.classs[i];
            if (!c._IsStruct()) continue;
            var o = c._GetInstance();

            if (c.Namespace != null && (i == 0 || (i > 0 && cfg.classs[i - 1].Namespace != c.Namespace))) // namespace 去重
            {
                sb.Append(@"
namespace " + c.Namespace.Replace(".", "::") + @" {");
            }

            sb.GenH_Struct(c, o);

            if (c.Namespace != null && ((i < cfg.classs.Count - 1 && cfg.classs[i + 1].Namespace != c.Namespace) || i == cfg.classs.Count - 1)) {
                sb.Append(@"
}");
            }
        }


        for (int i = 0; i < cfg.classs.Count; ++i) {
            var c = cfg.classs[i];
            if (c._IsStruct()) continue;
            var o = c._GetInstance();

            if (c.Namespace != null && (i == 0 || (i > 0 && cfg.classs[i - 1].Namespace != c.Namespace))) // namespace 去重
            {
                sb.Append(@"
namespace " + c.Namespace.Replace(".", "::") + @" {");
            }

            sb.GenH_Struct(c, o);

            if (c.Namespace != null && ((i < cfg.classs.Count - 1 && cfg.classs[i + 1].Namespace != c.Namespace) || i == cfg.classs.Count - 1)) {
                sb.Append(@"
}");
            }
        }


        sb.Append(@"
}
namespace xx {");
        foreach (var c in cfg.classs) {
            if (!c._IsStruct()) continue;
            var ctn = c._GetTypeDecl_Cpp();
            sb.Append(@"
	XX_GENCODE_STRUCT_TEMPLATE_H(" + ctn + @")");
        }
        sb.Append(@"
}");

        sb.Append(@"
#include """ + cfg.name + @"_class_lite_.h.inc""  // user create it for extend include files at the end
");
        createEmptyFiles.Add(cfg.name + "_class_lite_.h.inc");


        sb._WriteToFile(Path.Combine(cfg.outdir_cpp, cfg.name + ".h"));
    }

    static void GenH_Struct(this StringBuilder sb, Type c, object o) {
        // 定位到基类
        var bt = c.BaseType;


        if (c._IsStruct()) {
            var btn = c._HasBaseType() ? (" : " + bt._GetTypeDecl_Cpp()) : "";
            sb.Append(c._GetDesc()._GetComment_Cpp(4) + @"
    struct " + c.Name + btn + @" {
        XX_GENCODE_STRUCT_H(" + c.Name + @")");
        }
        else {
            var btn = c._HasBaseType() ? bt._GetTypeDecl_Cpp() : "::xx::ObjBase";
            sb.Append(c._GetDesc()._GetComment_Cpp(4) + @"
    struct " + c.Name + " : " + btn + @" {
        XX_GENCODE_OBJECT_H(" + c.Name + @", " + btn + @")");
        }


        if (c._Has<TemplateLibrary.Include>()) {
            sb.Append(@"
#include """ + c._GetTypeDecl_Lua() + @".inc""");
            createEmptyFiles.Add(c._GetTypeDecl_Lua() + ".inc");
        }

        var fs = c._GetFieldsConsts();
        foreach (var f in fs) {
            var ft = f.FieldType;
            var ftn = ft._GetTypeDecl_Cpp();
            sb.Append(f._GetDesc()._GetComment_Cpp(8) + @"
        " + (f.IsStatic ? "constexpr " : "") + ftn + " " + f.Name);

            var v = f.GetValue(f.IsStatic ? null : o);
            var dv = ft._GetDefaultValueDecl_Cpp(v);
            if (dv != "") {
                sb.Append(" = " + dv + ";");
            }
            else {
                sb.Append(";");
            }
        }

        if (c._Has<TemplateLibrary.Include_>()) {
            sb.Append(@"
#include """ + c._GetTypeDecl_Lua() + @"_.inc""");
            createEmptyFiles.Add(c._GetTypeDecl_Lua() + ".inc");
        }

        sb.Append(@"
    };");
    }

    public static void Gen_cpp() {
        var sb = new StringBuilder();
        sb.Append("#include \"" + cfg.name + @".h""
#include """ + cfg.name + @".cpp.inc""");
        createEmptyFiles.Add(cfg.name + ".cpp.inc");

        sb.Append(@"
namespace " + cfg.name + @" {
	void CodeGenTypes::Register() {");
        foreach (var kv in cfg.typeIdClassMappings) {
            var ctn = kv.Value._GetTypeDecl_Cpp();
            sb.Append(@"
	    ::xx::ObjManager::Register<" + ctn + @">();");
        }
        sb.Append(@"
	}
}
");

        sb.Append(@"
namespace xx {");
        foreach (var c in cfg.classs) {
            if (!c._IsStruct()) continue;
            var o = c._GetInstance();

            var ctn = c._GetTypeDecl_Cpp();
            var fs = c._GetFields();
            sb.Append(@"
	void ObjFuncs<" + ctn + @", void>::Write(::xx::ObjManager& om, " + ctn + @" const& in) {");

            if (c._HasBaseType()) {
                var bt = c.BaseType;
                var btn = bt._GetTypeDecl_Cpp();
                sb.Append(@"
        ObjFuncs<" + btn + ">::Write(om, in);");
            }

            if (c._Has<TemplateLibrary.Compatible>()) {
                sb.Append(@"
        auto bak = om.data->WriteJump(sizeof(uint32_t));");
            }

            foreach (var f in fs) {
                var ft = f.FieldType;
                sb.Append(@"
        om.Write(in." + f.Name + ");");
            }

            if (c._Has<TemplateLibrary.Compatible>()) {
                sb.Append(@"
        om.data->WriteFixedAt(bak, (uint32_t)(om.data->len - bak));");
            }

            sb.Append(@"
    }");

            sb.Append(@"
	int ObjFuncs<" + ctn + @", void>::Read(::xx::ObjManager& om, " + ctn + @"& out) {");

            if (c._HasBaseType()) {
                var bt = c.BaseType;
                var btn = bt._GetTypeDecl_Cpp();

                sb.Append(@"
        if (int r = ObjFuncs<" + btn + ">::Read(om, out)) return r;");
            }

            if (c._Has<TemplateLibrary.Compatible>()) {
                sb.Append(@"
        uint32_t siz;
        if (int r = om.data->ReadFixed(siz)) return r;
        auto endOffset = om.data->offset + siz;
");
                foreach (var f in fs) {
                    var ft = f.FieldType;

                    string dv = "";
                    var v = f.GetValue(f.IsStatic ? null : o);
                    dv = ft._GetDefaultValueDecl_Cpp(v);
                    if (dv != "") {
                        dv = "out." + f.Name + " = " + dv;
                    }
                    else {
                        dv = "om.SetDefaultValue(out." + f.Name + ")";
                    }

                    sb.Append(@"
        if (om.data->offset >= endOffset) " + dv + @";
        else if (int r = om.Read(out." + f.Name + @")) return r;");
                }
            }
            else {
                foreach (var f in fs) {
                    sb.Append(@"
        if (int r = om.Read(out." + f.Name + @")) return r;");
                }
            }

            sb.Append(@"
        return 0;
    }");

            sb.Append(@"
	void ObjFuncs<" + ctn + @", void>::Append(ObjManager &om, " + ctn + @" const& in) {
        om.str->push_back('{');
        AppendCore(om, in);
        om.str->push_back('}');
    }
	void ObjFuncs<" + ctn + @", void>::AppendCore(ObjManager &om, " + ctn + @" const& in) {");
            if (c._HasBaseType()) {
                var bt = c.BaseType;
                var btn = bt._GetTypeDecl_Cpp();
                sb.Append(@"
        auto sizeBak = om.str->size();
        ObjFuncs<" + btn + ">::AppendCore(om, in);");
            }

            foreach (var f in fs) {
                var ft = f.FieldType;
                if (f == fs[0]) {
                    if (c._HasBaseType()) {
                        sb.Append(@"
        if (sizeBak < om.str->size()) {
            om.str->push_back(',');
        }");
                    }
                    sb.Append(@"
        om.Append(""\""" + f.Name + @"\"":"", in." + f.Name + @"); ");
                }
                else {
                    sb.Append(@"
        om.Append("",\""" + f.Name + @"\"":"", in." + f.Name + @");");
                }
            }
            sb.Append(@"
    }");

            sb.Append(@"
    void ObjFuncs<" + ctn + @">::Clone1(::xx::ObjManager& om, " + ctn + @" const& in, " + ctn + @" &out) {");
            if (c._HasBaseType()) {
                var bt = c.BaseType;
                var btn = bt._GetTypeDecl_Cpp();
                sb.Append(@"
        ObjFuncs<" + btn + ">::Clone1(om, in, out);");
            }
            foreach (var f in fs) {
                var ft = f.FieldType;
                sb.Append(@"
        om.Clone1(in." + f.Name + ", out." + f.Name + ");");
            }
            sb.Append(@"
    }");

            sb.Append(@"
    void ObjFuncs<" + ctn + @">::Clone2(::xx::ObjManager& om, " + ctn + @" const& in, " + ctn + @" &out) {");
            if (c._HasBaseType()) {
                var bt = c.BaseType;
                var btn = bt._GetTypeDecl_Cpp();
                sb.Append(@"
        ObjFuncs<" + btn + ">::Clone2(om, in, out);");
            }
            foreach (var f in fs) {
                var ft = f.FieldType;
                sb.Append(@"
        om.Clone2(in." + f.Name + ", out." + f.Name + ");");
            }
            sb.Append(@"
    }");

            sb.Append(@"
    int ObjFuncs<" + ctn + @">::RecursiveCheck(::xx::ObjManager& om, " + ctn + @" const& in) {");
            if (c._HasBaseType()) {
                var bt = c.BaseType;
                var btn = bt._GetTypeDecl_Cpp();
                sb.Append(@"
        if (int r = ObjFuncs<" + btn + ">::RecursiveCheck(om, in)) return r;");
            }
            foreach (var f in fs) {
                var ft = f.FieldType;
                sb.Append(@"
        if (int r = om.RecursiveCheck(in." + f.Name + ")) return r;");
            }
            sb.Append(@"
        return 0;
    }");

            sb.Append(@"
    void ObjFuncs<" + ctn + @">::RecursiveReset(::xx::ObjManager& om, " + ctn + @"& in) {");
            if (c._HasBaseType()) {
                var bt = c.BaseType;
                var btn = bt._GetTypeDecl_Cpp();
                sb.Append(@"
        ObjFuncs<" + btn + ">::RecursiveReset(om, in);");
            }
            foreach (var f in fs) {
                var ft = f.FieldType;
                sb.Append(@"
        om.RecursiveReset(in." + f.Name + ");");
            }
            sb.Append(@"
    }");

            sb.Append(@"
    void ObjFuncs<" + ctn + @">::SetDefaultValue(::xx::ObjManager& om, " + ctn + @"& in) {");
            if (c._HasBaseType()) {
                var bt = c.BaseType;
                var btn = bt._GetTypeDecl_Cpp();
                sb.Append(@"
        ObjFuncs<" + btn + ">::SetDefaultValue(om, in);");
            }
            foreach (var f in fs) {
                var ft = f.FieldType;

                string dv = "";
                var v = f.GetValue(f.IsStatic ? null : o);
                dv = ft._GetDefaultValueDecl_Cpp(v);
                if (dv != "") {
                    dv = "in." + f.Name + " = " + dv;
                }
                else {
                    dv = "om.SetDefaultValue(in." + f.Name + ")";
                }

                sb.Append(@"
        " + dv + @";");
            }
            sb.Append(@"
    }");


        }
        sb.Append(@"
}");

        sb.Append(@"
namespace " + cfg.name + @" {");

        for (int i = 0; i < cfg.classs.Count; ++i) {
            var c = cfg.classs[i];

            // namespace c_ns {
            if (c.Namespace != null && (i == 0 || (i > 0 && cfg.classs[i - 1].Namespace != c.Namespace))) // namespace 去重
            {
                sb.Append(@"
namespace " + c.Namespace.Replace(".", "::") + @" {");
            }

            var o = c._GetInstance();

            sb.Append(@"
    " + c.Name + @"::" + c.Name + @"(" + c.Name + @"&& o) noexcept {
        this->operator=(std::move(o));
    }
    " + c.Name + @"& " + c.Name + @"::operator=(" + c.Name + @"&& o) noexcept {");
            if (c._HasBaseType()) {
                var bt = c.BaseType;
                var btn = bt._GetTypeDecl_Cpp();
                sb.Append(@"
        this->" + (c._IsStruct() ? btn : "BaseType") + "::operator=(std::move(o));");
            }
            var fs = c._GetFields();
            foreach (var f in fs) {
                var ft = f.FieldType;
                sb.Append(@"
        std::swap(this->" + f.Name + ", o." + f.Name + ");");
            }
            sb.Append(@"
        return *this;
    }");

            if (c._IsClass()) {

                sb.Append(@"
    void " + c.Name + @"::Write(::xx::ObjManager& om) const {");

                if (c._HasBaseType()) {
                    var bt = c.BaseType;
                    sb.Append(@"
        this->BaseType::Write(om);");
                }

                if (c._Has<TemplateLibrary.Compatible>()) {
                    sb.Append(@"
        auto bak = om.data->WriteJump(sizeof(uint32_t));");
                }

                foreach (var f in fs) {
                    var ft = f.FieldType;
                    sb.Append(@"
        om.Write(this->" + f.Name + ");");
                }

                if (c._Has<TemplateLibrary.Compatible>()) {
                    sb.Append(@"
        om.data->WriteFixedAt(bak, (uint32_t)(om.data->len - bak));");
                }

                sb.Append(@"
    }");

                sb.Append(@"
    int " + c.Name + @"::Read(::xx::ObjManager& om) {");

                if (c._HasBaseType()) {
                    sb.Append(@"
        if (int r = this->BaseType::Read(om)) return r;");
                }

                if (c._Has<TemplateLibrary.Compatible>()) {
                    sb.Append(@"
        uint32_t siz;
        if (int r = om.data->ReadFixed(siz)) return r;
        auto endOffset = om.data->offset - sizeof(siz) + siz;
");
                    foreach (var f in fs) {
                        var ft = f.FieldType;

                        string dv = "";
                        var v = f.GetValue(f.IsStatic ? null : o);
                        dv = ft._GetDefaultValueDecl_Cpp(v);
                        if (dv != "") {
                            dv = "this->" + f.Name + " = " + dv;
                        }
                        else {
                            dv = "om.SetDefaultValue(this->" + f.Name + ")";
                        }

                        sb.Append(@"
        if (om.data->offset >= endOffset) " + dv + @";
        else if (int r = om.Read(this->" + f.Name + @")) return r;");
                    }

                    sb.Append(@"

        if (om.data->offset > endOffset) return __LINE__;
        else om.data->offset = endOffset;");
                }
                else {
                    foreach (var f in fs) {
                        sb.Append(@"
        if (int r = om.Read(this->" + f.Name + @")) return r;");
                    }
                }

                sb.Append(@"
        return 0;
    }");
                var ctn = c._GetTypeDecl_Cpp();
                sb.Append(@"
    void " + c.Name + @"::Append(::xx::ObjManager& om) const {
        om.Append(""{\""__typeId__\"":"", this->ObjBase::GetTypeId());");
                sb.Append(@"
		this->AppendCore(om);
		om.str->push_back('}');
    }
    void " + c.Name + @"::AppendCore(::xx::ObjManager& om) const {");

                if (c._HasBaseType()) {
                    var bt = c.BaseType;
                    sb.Append(@"
        this->BaseType::AppendCore(om);");
                }

                foreach (var f in fs) {
                    var ft = f.FieldType;
                    sb.Append(@"
        om.Append("",\""" + f.Name + @"\"":"", this->" + f.Name + @");");
                }
                sb.Append(@"
    }");

                sb.Append(@"
    void " + c.Name + @"::Clone1(::xx::ObjManager& om, void* const &tar) const {");
                if (c._HasBaseType()) {
                    var bt = c.BaseType;
                    sb.Append(@"
        this->BaseType::Clone1(om, tar);");
                }
                if (fs.Count > 0) {
                    sb.Append(@"
        auto out = (" + c._GetTypeDecl_Cpp() + @"*)tar;");
                }
                foreach (var f in fs) {
                    var ft = f.FieldType;
                    sb.Append(@"
        om.Clone1(this->" + f.Name + ", out->" + f.Name + ");");
                }
                sb.Append(@"
    }");
                sb.Append(@"
    void " + c.Name + @"::Clone2(::xx::ObjManager& om, void* const &tar) const {");
                if (c._HasBaseType()) {
                    var bt = c.BaseType;
                    sb.Append(@"
        this->BaseType::Clone2(om, tar);");
                }
                if (fs.Count > 0) {
                    sb.Append(@"
        auto out = (" + c._GetTypeDecl_Cpp() + @"*)tar;");
                }

                foreach (var f in fs) {
                    var ft = f.FieldType;
                    sb.Append(@"
        om.Clone2(this->" + f.Name + ", out->" + f.Name + ");");
                }
                sb.Append(@"
    }");

                sb.Append(@"
    int " + c.Name + @"::RecursiveCheck(::xx::ObjManager& om) const {");
                if (c._HasBaseType()) {
                    var bt = c.BaseType;
                    sb.Append(@"
        if (int r = this->BaseType::RecursiveCheck(om)) return r;");
                }
                foreach (var f in fs) {
                    var ft = f.FieldType;
                    // todo: 跳过不含有 Shared 的类型的生成
                    sb.Append(@"
        if (int r = om.RecursiveCheck(this->" + f.Name + ")) return r;");
                }
                sb.Append(@"
        return 0;
    }");

                sb.Append(@"
    void " + c.Name + @"::RecursiveReset(::xx::ObjManager& om) {");
                if (c._HasBaseType()) {
                    var bt = c.BaseType;
                    sb.Append(@"
        this->BaseType::RecursiveReset(om);");
                }
                foreach (var f in fs) {
                    var ft = f.FieldType;
                    sb.Append(@"
        om.RecursiveReset(this->" + f.Name + ");");
                }
                sb.Append(@"
    }");


                sb.Append(@"
    void " + c.Name + @"::SetDefaultValue(::xx::ObjManager& om) {");
                if (c._HasBaseType()) {
                    var bt = c.BaseType;
                    sb.Append(@"
        this->BaseType::SetDefaultValue(om);");
                }
                foreach (var f in fs) {
                    var ft = f.FieldType;

                    string dv = "";
                    var v = f.GetValue(f.IsStatic ? null : o);
                    dv = ft._GetDefaultValueDecl_Cpp(v);
                    if (dv != "") {
                        dv = "this->" + f.Name + " = " + dv;
                    }
                    else {
                        dv = "om.SetDefaultValue(this->" + f.Name + ")";
                    }

                    sb.Append(@"
        " + dv + @";");
                }
                sb.Append(@"
    }");


            }

            // namespace }
            if (c.Namespace != null && ((i < cfg.classs.Count - 1 && cfg.classs[i + 1].Namespace != c.Namespace) || i == cfg.classs.Count - 1)) {
                sb.Append(@"
}");
            }
        }

        sb.Append(@"
}
");    // namespace _templateName

        sb._WriteToFile(Path.Combine(cfg.outdir_cpp, cfg.name + ".cpp"));

    }

    static void GenH_AJSON(this StringBuilder sb, Type c) {
        if (c._HasBaseType()) {
            GenH_AJSON(sb, c.BaseType);
        }
        var fs = c._GetFields();
        foreach (var f in fs) {
            sb.Append(", " + f.Name);
        }
    }

    public static void Gen_ajson_h() {
        var sb = new StringBuilder();
        sb.Append(@"#pragma once
#include """ + cfg.name + @".h""
#include ""ajson.hpp""");
        foreach (var c in cfg.localClasssStructs) {
            if (c._HasClassMember()) continue;
            sb.Append(@"
AJSON(" + "::" + c.Name);
            GenH_AJSON(sb, c);
            sb.Append(");");
        }
        sb.Append(@"
");
        sb._WriteToFile(Path.Combine(cfg.outdir_cpp, cfg.name + "_ajson.h"));
    }

    public static void Gen_empties() {
        var sb = new StringBuilder();
        foreach (var fn in createEmptyFiles) {
            var path = Path.Combine(cfg.outdir_cpp, fn);
            if (File.Exists(path)) {
                Console.ForegroundColor = ConsoleColor.DarkGreen;
                Console.WriteLine("已跳过 " + fn);
                Console.ResetColor();
                continue;
            }
            sb._WriteToFile(path);
        }
    }
}

using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using System.Linq;
using System.Collections.Generic;


/*

配置文件示例： 

.\shareds 目录下文件列表：gen_cfg.json abc.cs def.cs
{
	"refs":[]
	"name":"Shared"								// 多用于生成物文件前缀
	"files":[".\abc.cs", ".\def.cs"],			// 合并编译 asm 的文件列表( 其路径相对于 gen_cfg.json 所在目录 或填写绝对路径 )
	"outdir_cs":"..\out\cs\"					// 对应语言的生成物 输出目录( 缺失 或 留空 表示不生成 ). 必须是已经存在的目录，生成器不帮忙创建
	"outdir_lua":"..\out\lua\"					//
	"outdir_cpp":"..\out\cpp\"					//
}

.\projs\p1 目录下文件列表：gen_cfg.json efg.cs hhhh.cs
{
	"refs":["..\..\shareds\gen_cfg.json"],		// 依赖项目配置。其 types 走 External 规则( 其路径相对于 gen_cfg.json 所在目录 或填写绝对路径 )
	"name":"P1",
	"files":[".\efg.cs", ".\hhhh.cs"],
	"outdir_cs":"..\..\..\out\cs\"
	"outdir_lua":"..\..\..\out\lua\"
	"outdir_cpp":"..\..\..\out\cpp\"
}

.\projs\p2 目录下文件列表：gen_cfg.json ffffff.cs
{
	"refs":["..\..\shareds\gen_cfg.json", "..\p1\gen_cfg.json"],	// 多个则先合并 types 再 ...
	"name":"P2",
	"files":[".\ffffff.cs"],
	"outdir_cs":"..\..\..\out\cs\"
	"outdir_lua":"..\..\..\out\lua\"
	"outdir_cpp":"..\..\..\out\cpp\"
}


生成命令行示例：

..\tools_codegen_src\bin\Debug\net5.0\tools_codegen_src.exe .\projs\p1\gen_cfg.json
..\tools_codegen_src\bin\Debug\net5.0\tools_codegen_src.exe .\projs\p2\gen_cfg.json
 
 
 */


// 这部分映射 json 结构
public partial class Cfg {
    public List<string> refs { get; set; }
    public string name { get; set; }
    public List<string> files { get; set; }
    public string outdir_cs { get; set; }
    public string outdir_lua { get; set; }
    public string outdir_cpp { get; set; }
}

// 常用生成器需求功能扩展
partial class Cfg {

    /// <summary>
    /// refs 指向的 cfgs
    /// </summary>
    public List<Cfg> refsCfgs = new List<Cfg>();
    /// <summary>
    /// files 编译后的 assembly
    /// </summary>
    public Assembly asm;
    /// <summary>
    /// asm 中所有用户类型( 本地&外部 class & struct & enums & interfaces )
    /// </summary>
    public List<Type> types;
    /// <summary>
    /// asm 中所有用户类型( 本地 class )
    /// </summary>
    public List<Type> localClasss;
    /// <summary>
    /// asm 中所有用户类型( 本地 struct )
    /// </summary>
    public List<Type> localStructs;
    /// <summary>
    /// asm 中所有用户类型( 本地 enum )
    /// </summary>
    public List<Type> localEnums;
    /// <summary>
    /// asm 中所有用户类型( 本地 interface )
    /// </summary>
    public List<Type> localInterfaces;
    /// <summary>
    /// asm 中所有用户类型( 本地 class & struct )
    /// </summary>
    public List<Type> localClasssStructs;
    /// <summary>
    /// asm 中所有用户类型( 本地 class & struct & enum )
    /// </summary>
    public List<Type> localClasssStructsEnums;
    /// <summary>
    /// asm 中所有用户类型( 本地 class & struct & enum & interface )
    /// </summary>
    public List<Type> localClasssStructsEnumsInterfaces;
    /// <summary>
    /// asm 中所有用户类型( 外部 class )
    /// </summary>
    public List<Type> externalClasss;
    /// <summary>
    /// asm 中所有用户类型( 外部 struct )
    /// </summary>
    public List<Type> externalStructs;
    /// <summary>
    /// asm 中所有用户类型( 外部 enum )
    /// </summary>
    public List<Type> externalEnums;
    /// <summary>
    /// asm 中所有用户类型( 外部 interface )
    /// </summary>
    public List<Type> externalInterfaces;
    /// <summary>
    /// asm 中所有用户类型( 外部 class & struct )
    /// </summary>
    public List<Type> externalClasssStructs;
    /// <summary>
    /// asm 中所有用户类型( 外部 class & struct & enum )
    /// </summary>
    public List<Type> externalClasssStructsEnums;
    /// <summary>
    /// asm 中所有用户类型( 外部 class & struct & enum & interface )
    /// </summary>
    public List<Type> externalClasssStructsEnumsInterfaces;
    /// <summary>
    /// asm 中所有用户类型( 本地&外部 class )
    /// </summary>
    public List<Type> classs;
    /// <summary>
    /// asm 中所有用户类型( 本地&外部 struct )
    /// </summary>
    public List<Type> structs;
    /// <summary>
    /// asm 中所有用户类型( 本地&外部 class & struct )
    /// </summary>
    public List<Type> classsStructs;
    /// <summary>
    /// asm 中所有用户类型( 本地&外部 enum )
    /// </summary>
    public List<Type> enums;
    /// <summary>
    /// asm 中所有用户类型( 本地&外部 interface )
    /// </summary>
    public List<Type> interfaces;
    /// <summary>
    /// typeId : type 字典，包含 asm 中所有用户类型( 本地&外部 填写了 [TypeId( ? )] 的 )
    /// </summary>
    public Dictionary<ushort, Type> typeIdClassMappings = new Dictionary<ushort, Type>();
    /// <summary>
    /// type : typeId 字典，包含 asm 中所有用户类型( 本地&外部 填写了 [TypeId( ? )] 的 )
    /// </summary>
    public Dictionary<Type, ushort> ClassTypeIdMappings = new Dictionary<Type, ushort>();


    /// <summary>
    /// 是否本地类型
    /// </summary>
    public bool IsLocal(Type t) {
        return localClasssStructsEnums.Contains(t);
    }
    /// <summary>
    /// 是否扩展类型
    /// </summary>
    public bool IsExternal(Type t) {
        return externalClasssStructsEnums.Contains(t);
    }
    /// <summary>
    /// 是否 class
    /// </summary>
    public bool IsClass(Type t) {
        return classs.Contains(t);
    }
    /// <summary>
    /// 是否 struct
    /// </summary>
    public bool IsStruct(Type t) {
        return structs.Contains(t);
    }
    /// <summary>
    /// 是否 struct
    /// </summary>
    public bool IsEnum(Type t) {
        return enums.Contains(t);
    }
    // todo: more: IsNullable  IsTuple IsList IsNumeric IsContainer GetChildType ........
}
















/*********************************************************************************************************/
/*********************************************************************************************************/
/*********************************************************************************************************/

// 初始化功能扩展
partial class Cfg {
    // 用于查找引用
    public static Dictionary<string, Cfg> allConfigs = new Dictionary<string, Cfg>();

    // json -> type
    public static T DeSerialize<T>(string json) {
        T obj = Activator.CreateInstance<T>();
        using (MemoryStream ms = new MemoryStream(System.Text.Encoding.UTF8.GetBytes(json))) {
            System.Runtime.Serialization.Json.DataContractJsonSerializer serializer = new System.Runtime.Serialization.Json.DataContractJsonSerializer(obj.GetType());
            return (T)serializer.ReadObject(ms);
        }
    }

    // json -> assembly
    public static Assembly CreateAssembly(string asmName, IEnumerable<string> fileNames) {
        var dllPath = RuntimeEnvironment.GetRuntimeDirectory();

        var compilation = CSharpCompilation.Create(asmName)
            .WithOptions(new CSharpCompilationOptions(OutputKind.DynamicallyLinkedLibrary))
            .AddReferences(MetadataReference.CreateFromFile(typeof(object).GetTypeInfo().Assembly.Location)
                , MetadataReference.CreateFromFile(typeof(Console).GetTypeInfo().Assembly.Location)
                , MetadataReference.CreateFromFile(typeof(TemplateLibrary.TypeId).GetTypeInfo().Assembly.Location)
                , MetadataReference.CreateFromFile(Path.Combine(dllPath, "mscorlib.dll"))
                , MetadataReference.CreateFromFile(Path.Combine(dllPath, "netstandard.dll"))
                , MetadataReference.CreateFromFile(Path.Combine(dllPath, "System.Runtime.dll")))
            .AddSyntaxTrees(from fn in fileNames select CSharpSyntaxTree.ParseText(@"
#pragma warning disable 0169, 0414
" + File.ReadAllText(fn)));

        foreach (var d in compilation.GetDiagnostics()) {
            Console.WriteLine(d);
        }
        using (var m = new MemoryStream()) {
            var r = compilation.Emit(m);
            if (r.Success) {
                m.Seek(0, SeekOrigin.Begin);
                return AssemblyLoadContext.Default.LoadFromStream(m);
            }
            return null;
        }
    }

    // 递归合并 files 填充到 list
    public static void FillCsFiles(Cfg cfg, ref List<string> list) {
        list.AddRange(cfg.files);
        foreach (var rc in cfg.refsCfgs) {
            FillCsFiles(rc, ref list);
        }
    }

    // 检查 tar 在 ref 树中是否存在
    public bool RecursiveRefExists(Cfg tar) {
        foreach (var c in refsCfgs) {
            if (c.RecursiveRefExists(tar)) return true;
        }
        return false;
    }

    // json -> cfg instance
    public static Cfg ReadFrom(string fn) {
        // 转为完全路径
        fn = Path.GetFullPath(fn);

        // 如果已创建配置就直接短路返回
        if (allConfigs.ContainsKey(fn)) return allConfigs[fn];

        // 检查文件是否存在
        if (!File.Exists(fn)) {
            Console.WriteLine("can't open code gen config json file: ", fn);
            Environment.Exit(-1);
        }

        // 备份并修改 工作目录 为 cfg 所在目录
        var cd = Environment.CurrentDirectory;
        Environment.CurrentDirectory = new FileInfo(fn).Directory.FullName;

        // 反序列化配置创建类实例
        var cfg = DeSerialize<Cfg>(File.ReadAllText(fn));

        // 放入字典去重
        allConfigs.Add(fn, cfg);

        // 如果有填写 refs, 就加载
        if (cfg.refs != null) {
            foreach (var s in cfg.refs) {
                var c = ReadFrom(s);
                if (!cfg.refsCfgs.Contains(c)) {
                    cfg.refsCfgs.Add(c);
                }
            }
        }

        // 检查循环引用
        if (cfg.RecursiveRefExists(cfg)) throw new Exception("recursive ref?");

        // 检查名字是否不合规
        if (string.IsNullOrWhiteSpace(cfg.name) || cfg.name.Trim() != cfg.name) throw new Exception("bad name: " + cfg.name);

        // 检查模板文件列表
        if (cfg.files == null || cfg.files.Count == 0) throw new Exception("miss files?");


        // 转为完整路径
        for (int i = 0; i < cfg.files.Count; i++) {
            cfg.files[i] = Path.GetFullPath(cfg.files[i]);
        }


        // 输出路径检查
        if (string.IsNullOrWhiteSpace(cfg.outdir_cs)
            && string.IsNullOrWhiteSpace(cfg.outdir_lua)
            && string.IsNullOrWhiteSpace(cfg.outdir_cpp)) throw new Exception("miss outdir_cs | outdir_lua | outdir_cpp ?");

        if (!string.IsNullOrWhiteSpace(cfg.outdir_cs)) {
            cfg.outdir_cs = Path.GetFullPath(cfg.outdir_cs);
            if (!Directory.Exists(cfg.outdir_cs)) throw new Exception("can't find outdir_cs dir: " + cfg.outdir_cs);
        }
        if (!string.IsNullOrWhiteSpace(cfg.outdir_lua)) {
            cfg.outdir_lua = Path.GetFullPath(cfg.outdir_lua);
            if (!Directory.Exists(cfg.outdir_lua)) throw new Exception("can't find outdir_lua dir: " + cfg.outdir_lua);
        }
        if (!string.IsNullOrWhiteSpace(cfg.outdir_cpp)) {
            cfg.outdir_cpp = Path.GetFullPath(cfg.outdir_cpp);
            if (!Directory.Exists(cfg.outdir_cpp)) throw new Exception("can't find outdir_cpp dir: " + cfg.outdir_cpp);
        }

        // 合并所有 refsCfgs 的所有源代码文件列表
        var allCsFiles = new List<string>();
        FillCsFiles(cfg, ref allCsFiles);
        allCsFiles = allCsFiles.Distinct().ToList();

        // 编译出 assembly
        cfg.asm = CreateAssembly(cfg.name, allCsFiles);
        if (cfg.asm == null) throw new Exception("compile error.");

        // 得到所有 class & struct & enum & interface ( 含有当前 asm 以及 refs asm 的所有类型 )
        cfg.types = cfg.asm.GetTypes().Where(
            t => t.Namespace != nameof(System) && t.Namespace != nameof(TemplateLibrary)
        ).ToList();

        // 归并 refs asm 所有 types
        var allExts = new List<Type>();
        foreach (var rc in cfg.refsCfgs) {
            allExts.AddRange(rc.types);
        }
        // 得到所有 type 的名称( 直接对比 type 的话，跨 asm 会不同 )
        var allExtNames = allExts.Select(o => o.FullName).Distinct().ToList();


        // 本地 所有 类型 = types 除开 allExts 以及明确标记为外部类型的
        cfg.localClasssStructsEnumsInterfaces = cfg.types.Where(o => !allExtNames.Contains(o.FullName) && !o._Has<TemplateLibrary.External>()).ToList();

        // 外部 所有 类型 = types 除开 本地 的
        cfg.externalClasssStructsEnumsInterfaces = cfg.types.Where(o => !cfg.localClasssStructsEnumsInterfaces.Contains(o)).ToList();


        // 所有 枚举 = types 过滤 是 枚举
        cfg.enums = cfg.types.Where(o => o.IsEnum).ToList();

        // 所有 接口 = types 过滤 是 接口
        cfg.interfaces = cfg.types.Where(o => o.IsInterface).ToList();

        // 所有 类 = types 过滤 是 类 & 未标记 是结构
        cfg.classs = cfg.types.Where(o => o.IsClass && !o._Has<TemplateLibrary.Struct>()).ToList();

        // 所有 结构 = types 过滤 是 类 & 标记 是结构 或 是 结构
        cfg.structs = cfg.types.Where(o => o.IsClass && o._Has<TemplateLibrary.Struct>() || o.IsValueType).ToList();


        // 所有 本地 枚举 = types 过滤 是 枚举
        cfg.localEnums = cfg.localClasssStructsEnumsInterfaces.Where(o => o.IsEnum).ToList();

        // 所有 本地 接口 = types 过滤 是 接口
        cfg.localInterfaces = cfg.localClasssStructsEnumsInterfaces.Where(o => o.IsInterface).ToList();

        // 所有 本地 类 = types 过滤 是 类 & 未标记 是结构
        cfg.localClasss = cfg.localClasssStructsEnumsInterfaces.Where(o => o.IsClass && !o._Has<TemplateLibrary.Struct>()).ToList();

        // 所有 本地 结构 = types 过滤 是 类 & 标记 是结构 或 是 结构
        cfg.localStructs = cfg.localClasssStructsEnumsInterfaces.Where(o => o.IsClass && o._Has<TemplateLibrary.Struct>() || o.IsValueType).ToList();


        // 所有 外部 枚举 = types 过滤 是 枚举
        cfg.externalEnums = cfg.externalClasssStructsEnumsInterfaces.Where(o => o.IsEnum).ToList();

        // 所有 外部 接口 = types 过滤 是 接口
        cfg.externalInterfaces = cfg.externalClasssStructsEnumsInterfaces.Where(o => o.IsInterface).ToList();

        // 所有 外部 类 = types 过滤 是 类 & 未标记 是结构
        cfg.externalClasss = cfg.externalClasssStructsEnumsInterfaces.Where(o => o.IsClass && !o._Has<TemplateLibrary.Struct>()).ToList();

        // 所有 外部 结构 = types 过滤 是 类 & 标记 是结构 或 是 结构
        cfg.externalStructs = cfg.externalClasssStructsEnumsInterfaces.Where(o => o.IsClass && o._Has<TemplateLibrary.Struct>() || o.IsValueType).ToList();

        // 各种合并
        cfg.classsStructs = new List<Type>();
        cfg.classsStructs.AddRange(cfg.classs);
        cfg.classsStructs.AddRange(cfg.structs);

        cfg.localClasssStructs = new List<Type>();
        cfg.localClasssStructs.AddRange(cfg.localClasss);
        cfg.localClasssStructs.AddRange(cfg.localStructs);
        cfg.localClasssStructsEnums = new List<Type>();
        cfg.localClasssStructsEnums.AddRange(cfg.localClasssStructs);
        cfg.localClasssStructsEnums.AddRange(cfg.localEnums);

        cfg.externalClasssStructs = new List<Type>();
        cfg.externalClasssStructs.AddRange(cfg.externalClasss);
        cfg.externalClasssStructs.AddRange(cfg.externalStructs);
        cfg.externalClasssStructsEnums = new List<Type>();
        cfg.externalClasssStructsEnums.AddRange(cfg.externalClasssStructs);
        cfg.externalClasssStructsEnums.AddRange(cfg.externalEnums);

        // 填充 typeId
        foreach (var c in cfg.classs) {
            var id = c._GetTypeId();
            if (id == null) { }// throw new Exception("type: " + c.FullName + " miss [TypeId(xxxxxx)]");
            else {
                if (cfg.typeIdClassMappings.ContainsKey(id.Value)) {
                    throw new Exception("type: " + c.FullName + "'s typeId is duplicated with " + cfg.typeIdClassMappings[id.Value].FullName);
                }
                else {
                    cfg.typeIdClassMappings.Add(id.Value, c);
                }
            }
        }
        foreach (var kv in cfg.typeIdClassMappings) {
            cfg.ClassTypeIdMappings.Add(kv.Value, kv.Key);
        }

        // todo: more 合法性检测( 标签加错位置? 值不对? .... )


        // todo: recursive refs check

        // 还原工作目录
        Environment.CurrentDirectory = cd;


        return cfg;
    }
}

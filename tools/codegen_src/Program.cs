using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using System.Linq;
using System.Collections.Generic;

public class Cfg {
    public List<string> refs { get; set; }
    public string name { get; set; }
    public List<string> files { get; set; }
    public string outdir_cs { get; set; }
    public string outdir_lua { get; set; }
    public string outdir_cpp { get; set; }
}

public static class Program {

    public static T DeSerialize<T>(string json) {
        T obj = Activator.CreateInstance<T>();
        using (MemoryStream ms = new MemoryStream(System.Text.Encoding.UTF8.GetBytes(json))) {
            System.Runtime.Serialization.Json.DataContractJsonSerializer serializer = new System.Runtime.Serialization.Json.DataContractJsonSerializer(obj.GetType());
            return (T)serializer.ReadObject(ms);
        }
    }


    public static Assembly CreateAssembly(IEnumerable<string> fileNames) {
        var dllPath = RuntimeEnvironment.GetRuntimeDirectory();

        var compilation = CSharpCompilation.Create("TmpLib")
            .WithOptions(new CSharpCompilationOptions(OutputKind.DynamicallyLinkedLibrary))
            .AddReferences(MetadataReference.CreateFromFile(typeof(object).GetTypeInfo().Assembly.Location)
                , MetadataReference.CreateFromFile(typeof(Console).GetTypeInfo().Assembly.Location)
                , MetadataReference.CreateFromFile(typeof(TypeHelpers).GetTypeInfo().Assembly.Location)
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

    public static void TipsAndExit(string msg, int exitCode = 0) {
        Console.WriteLine(msg);
        Console.WriteLine("按任意键退出");
        Console.ReadKey();
        Environment.Exit(exitCode);
    }

    public static void Main(string[] args) {

        //var cfg = DeSerialize<Cfg>(File.ReadAllText(args[0]));

        //// todo

        //return;

        if (args.Length < 3) {
            TipsAndExit(@"
*.cs -> codes 生成器使用提示：

缺参数：
    根命名空间,    输出目录,    源码文件清单( 完整路径, 可多个 )

输出目录:
    如果缺失，默认为 工作目录. 必须已存在, 不会帮忙创建

源码文件或目录清单:
    如果缺失，默认为 工作目录下 *.cs
");
        }

        var rootNamespace = args[0];

        var outPath = ".";
        var inPaths = new List<string>();

        if (args.Length > 1) {
            if (!Directory.Exists(args[1])) {
                TipsAndExit("参数2 错误：目录不存在");
            }
            outPath = args[1];
        }

        var fileNames = new HashSet<string>();
        for (int i = 2; i < args.Length; ++i) {
            if (!File.Exists(args[i])) {
                TipsAndExit("参数 " + i + 1 + " 错误：文件不存在: " + args[i]);
            }
            else {
                fileNames.Add(args[i]);
            }
        }

        var asm = CreateAssembly(fileNames);
        if (asm == null) return;

        Console.WriteLine("开始生成");
        try {
            GenCPP_Class_Lite.Gen(asm, outPath, rootNamespace);
        }
        catch (Exception ex) {
            TipsAndExit("生成失败: " + ex.Message + "\r\n" + ex.StackTrace);
        }
        TipsAndExit("生成完毕");


        //var fn = Path.Combine(Environment.CurrentDirectory, "Program.cs");
        //var asm = GetAssembly(new string[] { fn });
        //if (asm != null) {
        //    asm.GetType("Program").GetMethod("Test").Invoke(null, null);
        //}
        //else Console.ReadLine();
    }
}


/*
 
 
 


.\shareds 目录下文件列表：gen_cfg.json abc.cs def.cs
{
	"refs":[]
	"name":"Shared"								// 多用于生成物文件前缀
	"files":[".\abc.cs", ".\def.cs"],			// 合并编译 asm 的文件列表( 其路径相对于 gen_cfg.json 所在目录 或填写绝对路径 )
	"outdir_cs":"..\out\cs\"					// 对应语言的生成物 输出目录( 缺失 或 留空 表示不生成 )
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

..\tools_codegen_src\bin\Debug\net5.0\tools_codegen_src.exe .\projs\p1\gen_cfg.json
..\tools_codegen_src\bin\Debug\net5.0\tools_codegen_src.exe .\projs\p2\gen_cfg.json


 
 
 
 */
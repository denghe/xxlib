using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using System.Linq;
using System.Collections.Generic;

public static class Program {

    public static void TipsAndExit(string msg, int exitCode = 0) {
        Console.WriteLine(msg);
        Console.WriteLine("按任意键退出");
        Console.ReadKey();
        Environment.Exit(exitCode);
    }



    public static void Main(string[] args) {
        string cfgPath;

        // find config file location
        if (args.Length == 0) {
            cfgPath = Environment.CurrentDirectory;

            // vs2019 F5 run: work dir = src\bin\Debug\net5.0
            if (cfgPath.EndsWith(".net5.0")) {
                cfgPath = Path.Combine(cfgPath, "../../../");
            }
            Path.Combine(cfgPath, "gen_cfg.json");
        }
        else {
            // arg specify
            cfgPath = args[0];
        }

        // file -> cfg instance
        var cfg = Cfg.ReadFrom(cfgPath);



        //var rootNamespace = args[0];

        //var outPath = ".";
        //var inPaths = new List<string>();

        //if (args.Length > 1) {
        //    if (!Directory.Exists(args[1])) {
        //        TipsAndExit("参数2 错误：目录不存在");
        //    }
        //    outPath = args[1];
        //}

        //var fileNames = new HashSet<string>();
        //for (int i = 2; i < args.Length; ++i) {
        //    if (!File.Exists(args[i])) {
        //        TipsAndExit("参数 " + i + 1 + " 错误：文件不存在: " + args[i]);
        //    }
        //    else {
        //        fileNames.Add(args[i]);
        //    }
        //}

        //var asm = CreateAssembly(fileNames);
        //if (asm == null) return;

        //Console.WriteLine("开始生成");
        //try {
        //    GenCPP_Class_Lite.Gen(asm, outPath, rootNamespace);
        //}
        //catch (Exception ex) {
        //    TipsAndExit("生成失败: " + ex.Message + "\r\n" + ex.StackTrace);
        //}
        //TipsAndExit("生成完毕");


        //var fn = Path.Combine(Environment.CurrentDirectory, "Program.cs");
        //var asm = GetAssembly(new string[] { fn });
        //if (asm != null) {
        //    asm.GetType("Program").GetMethod("Test").Invoke(null, null);
        //}
        //else Console.ReadLine();
    }
}

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
        TypeHelpers.cfg = Cfg.ReadFrom(cfgPath);

        Console.WriteLine("开始生成");
        try {
            if (!string.IsNullOrWhiteSpace(TypeHelpers.cfg.outdir_cpp)) {
                GenCpp.Gen();
            }
            if (!string.IsNullOrWhiteSpace(TypeHelpers.cfg.outdir_cs)) {
                GenCS.Gen();
            }
            if (!string.IsNullOrWhiteSpace(TypeHelpers.cfg.outdir_lua)) {
                GenLua.Gen();
            }
        }
        catch (Exception ex) {
            TipsAndExit("生成失败: " + ex.Message + "\r\n" + ex.StackTrace);
        }
        TipsAndExit("生成完毕");
    }
}

using System;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;

public static class StringHelpers
{
    public static string MD5PlaceHolder = "#*MD5<>*#";
    public static string MD5PlaceHolder_Left = "#*MD5<";
    public static string MD5PlaceHolder_Right = ">*#";

    public static string GetMD5(this byte[] data)
    {
        var md5 = new MD5CryptoServiceProvider();
        var bytes = md5.ComputeHash(data);
        var sb = new StringBuilder();
        for (int i = 0; i < bytes.Length; i++)
        {
            sb.Append(bytes[i].ToString("x2"));
        }
        return sb.ToString();
    }

    public static string GetMD5(this string txt)
    {
        return GetMD5(Encoding.UTF8.GetBytes(txt));
    }

    public static string GetMD5(this StringBuilder sb)
    {
        return GetMD5(Encoding.UTF8.GetBytes(sb.ToString()));
    }





    /// <summary>
    /// 首字符转大写
    /// </summary>
    public static string _ToFirstUpper(this string s)
    {
        return s.Substring(0, 1).ToUpper() + s.Substring(1);
    }
    /// <summary>
    /// 首字符转小写
    /// </summary>
    public static string _ToFirstLower(this string s)
    {
        return s.Substring(0, 1).ToLower() + s.Substring(1);
    }

    /// <summary>
    /// 定位到数组最后一个元素
    /// </summary>
    public static T _Last<T>(this T[] a)
    {
        return a[a.Length - 1];
    }

    /// <summary>
    /// 定位到集合最后一个元素
    /// </summary>
    public static T _Last<T>(this List<T> a)
    {
        return a[a.Count - 1];
    }


    /// <summary>
    /// 以 utf8 格式写文本到文件, 可选择是否附加 bom 头.
    /// </summary>
    public static bool _Write(this string fn, StringBuilder sb, bool useBOM = true)
    {
        // 读旧文件内容
        string oldTxt = null;
        if (File.Exists(fn))
        {
            oldTxt = File.ReadAllText(fn);
        }

        // 算 md5 , 替换后对比
        var txt = sb.ToString();
        var md5 = StringHelpers.GetMD5(txt);
        txt = txt.Replace(StringHelpers.MD5PlaceHolder, StringHelpers.MD5PlaceHolder_Left + md5 + StringHelpers.MD5PlaceHolder_Right);
        if (oldTxt == txt)
        {
            Console.ForegroundColor = ConsoleColor.DarkGreen;
            Console.WriteLine("已跳过 " + fn);
            Console.ResetColor();
            return false;
        }

        // 新建或覆盖写入
        using (var fs = File.Create(fn))
        {
            if (useBOM)
            {
                fs.Write(_bom, 0, _bom.Length);
            }
            var buf = Encoding.UTF8.GetBytes(txt);
            fs.Write(buf, 0, buf.Length);
            fs.Close();
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine("已生成 " + fn);
            Console.ResetColor();
            return true;
        }
    }

    public static byte[] _bom = { 0xEF, 0xBB, 0xBF };

    /// <summary>
    /// 以 utf8 格式写文本到文件, 可选择是否附加 bom 头.
    /// </summary>
    public static bool _WriteToFile(this StringBuilder sb, string fn, bool useBOM = true)
    {
        return fn._Write(sb, useBOM);
    }
}

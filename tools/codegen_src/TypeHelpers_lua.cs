using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;


public static partial class TypeHelpers {

    /// <summary>
    /// 获取 LUA 的默认值填充代码
    /// </summary>
    public static string _GetDefaultValueDecl_Lua(this object v) {
        if (v == null) return "null";
        var t = v.GetType();
        if (t._IsNullable()) {
            return "";
        }
        else if (t.IsValueType) {
            if (t.IsEnum) {
                var sv = v._GetEnumInteger(t);
                if (sv == "0") return "0";
                // 如果 v 的值在枚举中找不到, 输出硬转格式. 否则输出枚举项
                var fs = t._GetEnumFields();
                if (fs.Exists(f => f._GetEnumValue(t).ToString() == sv)) {
                    return _GetTypeDecl_Lua(t) + "." + v.ToString();
                }
                else {
                    return sv.ToString();
                }
            }
            if (t._IsNumeric()) return v.ToString().ToLower();   // lower for Ture, False bool
            else return "";
        }
        else if (t._IsString()) {
            return "[[" + (string)v + "]]";
        }
        else {
            return v.ToString();
        }
        // todo: 其他需要引号的类型的处理, 诸如 DateTime, Guid 啥的
    }


    /// <summary>
    /// 获取 C++ 的类型声明串
    /// </summary>
    public static string _GetTypeDecl_Lua(this Type t) {
        if (t._IsNullable()) {
            return "Nullable" + _GetTypeDecl_Lua(t.GenericTypeArguments[0]);
        }
        else if (t._IsWeak()) {
            return "Weak_" + _GetTypeDecl_Lua(t.GenericTypeArguments[0]);
        }
        else if (t._IsList()) {
            string rtv = t.Name.Substring(0, t.Name.IndexOf('`')) + "_";
            for (int i = 0; i < t.GenericTypeArguments.Length; ++i) {
                if (i > 0)
                    rtv += "_";
                rtv += _GetTypeDecl_Lua(t.GenericTypeArguments[i]);
            }
            rtv += "_";
            return rtv;
        }
        else if (t.Namespace == nameof(System) || t.Namespace == nameof(TemplateLibrary)) {
            return t.Name;
        }
        return (t._IsExternal() ? "" : "") + "_" + t.FullName.Replace(".", "_");
    }



    /// <summary>
    /// 获取 LUA 风格的注释
    /// </summary>
    public static string _GetComment_Lua(this string s, int space) {
        if (s.Trim() == "")
            return "";
        var sps = new string(' ', space);
        return @"
" + sps + @"--[[
" + sps + s + @"
" + sps + "]]";
    }

}

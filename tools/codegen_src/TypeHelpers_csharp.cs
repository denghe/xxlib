using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;


public static partial class TypeHelpers {

    /// <summary>
    /// 获取 C# 的默认值填充代码
    /// </summary>
    public static string _GetDefaultValueDecl_Csharp(this object v) {
        if (v == null) return "";
        var t = v.GetType();
        if (t._IsNullable()) {
            return "";
        }
        else if (t.IsValueType) {
            if (t.IsEnum) {
                var sv = v._GetEnumInteger(t);
                if (sv == "0") return "";
                // 如果 v 的值在枚举中找不到, 输出硬转格式. 否则输出枚举项
                var fs = t._GetEnumFields();
                if (fs.Exists(f => f._GetEnumValue(t).ToString() == sv)) {
                    return t.FullName + "." + v.ToString();
                }
                else {
                    return "(" + _GetTypeDecl_Csharp(t) + ")" + v._GetEnumInteger(t);
                }
            }
            var s = v.ToString();
            if (s == "0") return "";
            if (s == "True" || s == "False") return s.ToLower();
            return "";
        }
        else if (t._IsString()) {
            return "@\"" + ((string)v).Replace("\"", "\"\"") + "\"";
        }
        else {
            return v.ToString();
        }
        // todo: 其他需要引号的类型的处理, 诸如 DateTime, Guid 啥的
    }

    /// <summary>
    /// 获取 C# 模板 的类型声明串
    /// </summary>
    public static string _GetTypeDecl_GenTypeIdTemplate(this Type t) {
        if (t._IsNullable()) {
            return t.GenericTypeArguments[0]._GetTypeDecl_GenTypeIdTemplate() + "?";
        }
        if (t.IsArray) {
            throw new NotSupportedException();
        }
        else if (t._IsTuple()) {
            string rtv = "Tuple<";
            for (int i = 0; i < t.GenericTypeArguments.Length; ++i) {
                if (i > 0)
                    rtv += ", ";
                rtv += _GetTypeDecl_GenTypeIdTemplate(t.GenericTypeArguments[i]);
            }
            rtv += ">";
            return rtv;
        }
        else if (t.IsEnum) {
            return t.FullName;
        }
        else {
            if (t.Namespace == nameof(TemplateLibrary)) {
                if (t.Name == "Weak`1") {
                    return "Weak<" + _GetTypeDecl_GenTypeIdTemplate(t.GenericTypeArguments[0]) + ">";
                }
                if (t.Name == "Shared`1") {
                    return "Shared<" + _GetTypeDecl_GenTypeIdTemplate(t.GenericTypeArguments[0]) + ">";
                }
                else if (t.Name == "List`1") {
                    return "List<" + _GetTypeDecl_GenTypeIdTemplate(t.GenericTypeArguments[0]) + ">";
                }
                else if (t.Name == "DateTime") {
                    return "DateTime";
                }
                else if (t.Name == "Data") {
                    return "Data";
                }
            }
            else if (t.Namespace == nameof(System)) {
                switch (t.Name) {
                    case "Object":
                        return "object";
                    case "Void":
                        return "void";
                    case "Byte":
                        return "byte";
                    case "UInt8":
                        return "byte";
                    case "UInt16":
                        return "ushort";
                    case "UInt32":
                        return "uint";
                    case "UInt64":
                        return "ulong";
                    case "SByte":
                        return "sbyte";
                    case "Int8":
                        return "sbyte";
                    case "Int16":
                        return "short";
                    case "Int32":
                        return "int";
                    case "Int64":
                        return "long";
                    case "Double":
                        return "double";
                    case "Float":
                        return "float";
                    case "Single":
                        return "float";
                    case "Boolean":
                        return "bool";
                    case "Bool":
                        return "bool";
                    case "String":
                        return "string";
                }
            }

            return t.FullName;
            //throw new Exception("unhandled data type");
        }
    }

    /// <summary>
    /// 获取 C# 的类型声明串
    /// </summary>
    public static string _GetTypeDecl_Csharp(this Type t) {
        if (t._IsNullable()) {
            return t.GenericTypeArguments[0]._GetTypeDecl_Csharp() + "?";
        }
        if (t.IsArray) {
            throw new NotSupportedException();
            //return _GetCSharpTypeDecl(t.GetElementType()) + "[]";
        }
        else if (t._IsTuple()) {
            string rtv = "Tuple<";
            for (int i = 0; i < t.GenericTypeArguments.Length; ++i) {
                if (i > 0)
                    rtv += ", ";
                rtv += _GetTypeDecl_Csharp(t.GenericTypeArguments[i]);
            }
            rtv += ">";
            return rtv;
        }
        else if (t.IsEnum) {
            return t.FullName;
        }
        else {
            if (t.Namespace == nameof(TemplateLibrary)) {
                if (t.Name == "Weak`1") {
                    return "xx.Weak<" + _GetTypeDecl_Csharp(t.GenericTypeArguments[0]) + ">";
                }
                if (t.Name == "Shared`1") {
                    return _GetTypeDecl_Csharp(t.GenericTypeArguments[0]);
                }
                else if (t.Name == "List`1") {
                    return "xx.List<" + _GetTypeDecl_Csharp(t.GenericTypeArguments[0]) + ">";
                }
                else if (t.Name == "Dict`2") {
                    return "xx.Dict<" + @"<" + t.GenericTypeArguments[0]._GetTypeDecl_Csharp() + ", " + t.GenericTypeArguments[1]._GetTypeDecl_Csharp() + ">";
                }
                else if (t.Name == "Pair`2") {
                    return "xx.Pair<" + @"<" + t.GenericTypeArguments[0]._GetTypeDecl_Csharp() + ", " + t.GenericTypeArguments[1]._GetTypeDecl_Csharp() + ">";
                }
                else if (t.Name == "DateTime") {
                    return "DateTime";
                }
                else if (t.Name == "Data") {
                    return "xx.Data";
                }
            }
            else if (t.Namespace == nameof(System)) {
                switch (t.Name) {
                    case "Object":
                        return "xx.Object";
                    case "Void":
                        return "void";
                    case "Byte":
                        return "byte";
                    case "UInt8":
                        return "byte";
                    case "UInt16":
                        return "ushort";
                    case "UInt32":
                        return "uint";
                    case "UInt64":
                        return "ulong";
                    case "SByte":
                        return "sbyte";
                    case "Int8":
                        return "sbyte";
                    case "Int16":
                        return "short";
                    case "Int32":
                        return "int";
                    case "Int64":
                        return "long";
                    case "Double":
                        return "double";
                    case "Float":
                        return "float";
                    case "Single":
                        return "float";
                    case "Boolean":
                        return "bool";
                    case "Bool":
                        return "bool";
                    case "String":
                        return "string";
                }
            }

            return t.FullName;
            //throw new Exception("unhandled data type");
        }
    }

    /// <summary>
    /// 获取 C# 的类型声明串( 显示用 )
    /// </summary>
    public static string _GetTypeDecl_CsharpForDisplayCppType(this Type t) {
        if (t._IsNullable()) {
            return t.GenericTypeArguments[0]._GetTypeDecl_CsharpForDisplayCppType() + "?";
        }
        if (t.IsArray) {
            // todo: 如果是 byte[] 则用于表达 xx.Data 对象
            throw new NotSupportedException();
            //return _GetCSharpTypeDecl(t.GetElementType()) + "[]";
        }
        else if (t._IsTuple()) {
            string rtv = "std::tuple<";
            for (int i = 0; i < t.GenericTypeArguments.Length; ++i) {
                if (i > 0)
                    rtv += ", ";
                rtv += _GetTypeDecl_CsharpForDisplayCppType(t.GenericTypeArguments[i]);
            }
            rtv += ">";
            return rtv;
        }
        else if (t.IsEnum) {
            return t.FullName;
        }
        else {
            if (t.Namespace == nameof(TemplateLibrary)) {
                if (t.Name == "Weak`1") {
                    return "xx::Weak<" + _GetTypeDecl_CsharpForDisplayCppType(t.GenericTypeArguments[0]) + ">";
                }
                if (t.Name == "Shared`1") {
                    return "xx::Shared<" + _GetTypeDecl_CsharpForDisplayCppType(t.GenericTypeArguments[0]) + ">";
                }
                else if (t.Name == "List`1") {
                    return "std::vector<" + _GetTypeDecl_CsharpForDisplayCppType(t.GenericTypeArguments[0]) + ">";
                }
                else if (t.Name == "Dict`2") {
                    return "std::map<" + _GetTypeDecl_CsharpForDisplayCppType(t.GenericTypeArguments[0]) + ", " + _GetTypeDecl_CsharpForDisplayCppType(t.GenericTypeArguments[1]) + ">";
                }
                else if (t.Name == "Pair`2") {
                    return "std::pair<" + _GetTypeDecl_CsharpForDisplayCppType(t.GenericTypeArguments[0]) + ", " + _GetTypeDecl_CsharpForDisplayCppType(t.GenericTypeArguments[1]) + ">";
                }
                //else if (t.Name == "DateTime") {
                //    return "DateTime";
                //}
                //else if (t.Name == "Data") {
                //    return "xx::Data";
                //}
            }
            else if (t.Namespace == nameof(System)) {
                switch (t.Name) {
                    case "Object":
                        return "xx.Object";
                    case "Void":
                        return "void";
                    case "Byte":
                        return "byte";
                    case "UInt8":
                        return "byte";
                    case "UInt16":
                        return "ushort";
                    case "UInt32":
                        return "uint";
                    case "UInt64":
                        return "ulong";
                    case "SByte":
                        return "sbyte";
                    case "Int8":
                        return "sbyte";
                    case "Int16":
                        return "short";
                    case "Int32":
                        return "int";
                    case "Int64":
                        return "long";
                    case "Double":
                        return "double";
                    case "Float":
                        return "float";
                    case "Single":
                        return "float";
                    case "Boolean":
                        return "bool";
                    case "Bool":
                        return "bool";
                    case "String":
                        return "string";
                }
            }

            return t.FullName;
            //throw new Exception("unhandled data type");
        }
    }

    /// <summary>
    /// 获取枚举对应的数字类型的类型名
    /// </summary>
    public static string _GetEnumUnderlyingTypeName_Csharp(this Type e) {
        switch (e.GetEnumUnderlyingType().Name) {
            case "Byte":
                return "byte";
            case "SByte":
                return "sbyte";
            case "UInt16":
                return "ushort";
            case "Int16":
                return "short";
            case "UInt32":
                return "uint";
            case "Int32":
                return "int";
            case "UInt64":
                return "ulong";
            case "Int64":
                return "long";
        }
        throw new Exception("unsupported data type");
    }

    /// <summary>
    /// 获取 C# 风格的注释
    /// </summary>
    public static string _GetComment_CSharp(this string s, int space) {
        if (s.Trim() == "")
            return "";
        var sps = new string(' ', space);
        s = s.Replace("\r\n", "\n")
         .Replace("\r", "\n")
         .Replace("\n", "\r\n" + sps + "/// ");
        return "\r\n" +
    sps + @"/// <summary>
" + sps + @"/// " + s + @"
" + sps + @"/// </summary>";
    }

}

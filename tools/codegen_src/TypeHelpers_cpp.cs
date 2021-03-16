using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;


public static partial class TypeHelpers {

    /// <summary>
    /// 获取 CPP 的默认值填充代码
    /// </summary>
    public static string _GetDefaultValueDecl_Cpp(this Type t, object v) {
        if (t._IsNullable()) {
            return v == null ? "" : v.ToString();
        }
        if (t.IsGenericType || t._IsData()) {
            return "";
        }
        if (t._IsString()) {
            return v == null ? "" : ("\"" + ((string)v).Replace("\"", "\"\"") + "\"");
        }
        if (t.IsValueType) {
            if (t.IsEnum) {
                var sv = v._GetEnumInteger(t);
                if (sv == "0") return "(" + _GetTypeDecl_Cpp(t) + ")0";
                // 如果 v 的值在枚举中找不到, 输出硬转格式. 否则输出枚举项
                var fs = t._GetEnumFields();
                if (fs.Exists(f => f._GetEnumValue(t).ToString() == sv)) {
                    return _GetTypeDecl_Cpp(t) + "::" + v.ToString();
                }
                else {
                    return "(" + _GetTypeDecl_Cpp(t) + ")" + v._GetEnumInteger(t);
                }
            }
            if (t._IsNumeric()) {
                if (t.Name == "Single") {
                    var s = v.ToString().ToLower();
                    if (s.Contains(".")) return s + "f";
                    return s + ".0f";
                }
                else
                    return v.ToString().ToLower();   // lower for Ture, False bool
            }
            else return "";
        }
        // class?
        return "";
    }

    /// <summary>
    /// 获取 C++ 的类型声明串
    /// </summary>
    public static string _GetTypeDecl_Cpp(this Type t) {
        if (t._IsNullable()) {
            return "::std::optional<" + t.GenericTypeArguments[0]._GetTypeDecl_Cpp() + ">";
        }
        if (t._IsData()) {
            return "::xx::Data";
        }
        else if (t._IsTuple()) {
            string rtv = "::std::tuple<";
            for (int i = 0; i < t.GenericTypeArguments.Length; ++i) {
                if (i > 0) {
                    rtv += ", ";
                }
                rtv += t.GenericTypeArguments[i]._GetTypeDecl_Cpp();
            }
            rtv += ">";
            return rtv;
        }
        else if (t.IsEnum)  // enum & struct
        {
            return "::" + (t._IsExternal() ? "" : ("::")) + t.FullName.Replace(".", "::");
        }
        else {
            if (t.Namespace == nameof(TemplateLibrary)) {
                switch (t.Name) {
                    case "Weak`1":
                        return "::xx::Weak<" + _GetTypeDecl_Cpp(t.GenericTypeArguments[0]) + ">";
                    case "Shared`1":
                        return "::xx::Shared<" + _GetTypeDecl_Cpp(t.GenericTypeArguments[0]) + ">";
                    case "Unique`1":
                        return "::std::unique_ptr<" + _GetTypeDecl_Cpp(t.GenericTypeArguments[0]) + ">";
                    case "List`1": {
                            var ct = t.GenericTypeArguments[0];
                            return "::std::vector" + @"<" + ct._GetTypeDecl_Cpp() + ">";
                        }
                    case "Dict`2": {
                            return "::std::map" + @"<" + t.GenericTypeArguments[0]._GetTypeDecl_Cpp() + ", " + t.GenericTypeArguments[1]._GetTypeDecl_Cpp() + ">";
                        }
                    case "Pair`2": {
                            return "::std::pair" + @"<" + t.GenericTypeArguments[0]._GetTypeDecl_Cpp() + ", " + t.GenericTypeArguments[1]._GetTypeDecl_Cpp() + ">";
                        }
                    case "Data":
                        return "::xx::Data";
                    default:
                        throw new NotImplementedException();
                }
            }
            else if (t.Namespace == nameof(System)) {
                switch (t.Name) {
                    case "Object":
                        return "::xx::ObjBase";
                    case "Void":
                        return "void";
                    case "Byte":
                        return "uint8_t";
                    case "UInt8":
                        return "uint8_t";
                    case "UInt16":
                        return "uint16_t";
                    case "UInt32":
                        return "uint32_t";
                    case "UInt64":
                        return "uint64_t";
                    case "SByte":
                        return "int8_t";
                    case "Int8":
                        return "int8_t";
                    case "Int16":
                        return "int16_t";
                    case "Int32":
                        return "int32_t";
                    case "Int64":
                        return "int64_t";
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
                        return "::std::string";
                }
            }
            return "::" + (t._IsExternal() ? "" : ("::")) + t.FullName.Replace(".", "::");
            //return (t._IsExternal() ? "" : ("::" + templateName)) + "::" + t.FullName.Replace(".", "::") + (t.IsValueType ? "" : ((t._IsExternal() && !t._GetExternalSerializable()) ? "" : suffix));
        }
    }

    /// <summary>
    /// 获取枚举对应的数字类型的类型名之 cpp 版
    /// </summary>
    public static string _GetEnumUnderlyingTypeName_Cpp(this Type e) {
        switch (e.GetEnumUnderlyingType().Name) {
            case "Byte":
                return "uint8_t";
            case "SByte":
                return "int8_t";
            case "UInt16":
                return "uint16_t";
            case "Int16":
                return "int16_t";
            case "UInt32":
                return "uint32_t";
            case "Int32":
                return "int32_t";
            case "UInt64":
                return "uint64_t";
            case "Int64":
                return "int64_t";
        }
        throw new Exception("unsupported data type");
    }

    /// <summary>
    /// 获取 Cpp 风格的注释
    /// </summary>
    public static string _GetComment_Cpp(this string s, int space) {
        if (s.Trim() == "")
            return "";
        var sps = new string(' ', space);
        s = s.Replace("\r\n", "\n")
         .Replace("\r", "\n")
         .Replace("\n", "\r\n" + sps + "// ");
        return "\r\n"
 + sps + @"// " + s;
    }
}

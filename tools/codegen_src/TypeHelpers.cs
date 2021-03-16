using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;


public static partial class TypeHelpers {

    // 缓存当前 cfg 以简化传参
    public static Cfg cfg;

    /**************************************************************************************************/
    // Is 系列
    /**************************************************************************************************/

    /// <summary>
    /// 返回 t 是否为 string
    /// </summary>
    public static bool _IsString(this Type t) {
        return t.Namespace == nameof(System) && t.Name == nameof(String);
    }

    /// <summary>
    /// 返回 t 是否为 object
    /// </summary>
    public static bool _IsObject(this Type t) {
        return t.Namespace == nameof(System) && t.Name == nameof(Object);
    }

    /// <summary>
    /// 返回 t 是否为 Weak
    /// </summary>
    public static bool _IsWeak(this Type t) {
        return t.Namespace == nameof(TemplateLibrary) && t.Name == "Weak`1";
    }

    /// <summary>
    /// 返回 t 是否为 Weak
    /// </summary>
    public static bool _IsUnique(this Type t) {
        return t.Namespace == nameof(TemplateLibrary) && t.Name == "Unique`1";
    }

    /// <summary>
    /// 返回 t 是否为 Shared
    /// </summary>
    public static bool _IsShared(this Type t) {
        return t.Namespace == nameof(TemplateLibrary) && t.Name == "Shared`1";
    }

    /// <summary>
    /// 返回 t 是否为 Data
    /// </summary>
    public static bool _IsData(this Type t) {
        return t.IsArray && t.Name == "Byte[]";
    }

    /// <summary>
    /// 返回 t 是否为容器( string, data, list )
    /// </summary>
    public static bool _IsContainer(this Type t) {
        if (t._IsNullable()) {
            t = t._GetChildType();
        }
        return t._IsString() || t._IsList() || t._IsData();
    }

    /// <summary>
    /// 返回泛型第 index 个子类型
    /// </summary>
    public static Type _GetChildType(this Type t, int index = 0) {
        return t.GenericTypeArguments[index];
    }

    /// <summary>
    /// 返回 t 是否为 类
    /// </summary>
    public static bool _IsClass(this Type t) {
        return cfg.classs.Contains(t);
    }

    /// <summary>
    /// 返回 t 是否为 结构体( 或 class 带 [Struct] 标记 )
    /// </summary>
    public static bool _IsStruct(this Type t) {
        return cfg.structs.Contains(t);
    }

    /// <summary>
    /// 返回 t 是否为外部扩展类型
    /// </summary>
    public static bool _IsExternal(this Type t) {
        return cfg.externalClasssStructsEnums.Contains(t);
    }

    /// <summary>
    /// 返回 t 是否为 void
    /// </summary>
    public static bool _IsVoid(this Type t) {
        return t.Namespace == nameof(System) && t.Name == "Void";
    }

    /// <summary>
    /// 返回 t 是否为 数字类型( byte ~ int64, float, double )
    /// </summary>
    public static bool _IsNumeric(this Type t) {
        return t.Namespace == nameof(System) && t.IsValueType && (
                t.Name == "Byte" ||
                t.Name == "UInt8" ||
                t.Name == "UInt16" ||
                t.Name == "UInt32" ||
                t.Name == "UInt64" ||
                t.Name == "SByte" ||
                t.Name == "Int8" ||
                t.Name == "Int16" ||
                t.Name == "Int32" ||
                t.Name == "Int64" ||
                t.Name == "Double" ||
                t.Name == "Single" ||
                t.Name == "Boolean" ||
                t.Name == "Bool"
            );
    }

    /// <summary>
    /// 返回 t 是否为 C# 中的 Nullable<>
    /// </summary>
    public static bool _IsNullable(this Type t) {
        return t.IsGenericType && (t.Namespace == nameof(System) || t.Namespace == nameof(TemplateLibrary)) && t.Name == "Nullable`1";
    }

    /// <summary>
    /// 返回 t 是否为 Tuple<........>
    /// </summary>
    public static bool _IsTuple(this Type t) {
        return t.IsGenericType && t.Namespace == nameof(TemplateLibrary) && t.Name.StartsWith("Tuple`");
    }

    /// <summary>
    /// 返回 t 是否为 List<T>
    /// </summary>
    public static bool _IsList(this Type t) {
        return t.IsGenericType && t.Namespace == nameof(TemplateLibrary) && t.Name == "List`1";
    }

    /// <summary>
    /// 返回 t 是否为 List<Shared<?>>>
    /// </summary>
    public static bool _IsListShared(this Type t) {
        return t._IsList() && t.GetGenericArguments()[0]._IsShared();
    }

    /**************************************************************************************************/
    // Has 系列
    /**************************************************************************************************/

    /// <summary>
    /// 判断目标类型是否为派生类
    /// </summary>
    public static bool _HasBaseType(this Type t) {
        if (t.BaseType == null) return false;
        return t.BaseType != typeof(object) && t.BaseType != typeof(System.ValueType);
    }

    /// <summary>
    /// 判断目标上是否有附加某个类型的 Attribute
    /// </summary>
    public static bool _Has<T>(this ICustomAttributeProvider f) {
        var cas = f.GetCustomAttributes(false).ToList();
        return cas.Any(a => a is T);
    }

    /// <summary>
    /// 递归判断 是否有任意成员变量类型是 class ( 含泛型 )
    /// </summary>
    public static bool _HasClassMember(this Type t) {
        if (t._IsExternal() && t.IsValueType) return false;
        if (t._IsClass()) return true;
        if (t._IsStruct()) {
            foreach (var m in t._GetFields()) {
                if (m.FieldType._HasClassMember()) return true;
            }
            return false;
        }
        if (t.IsGenericType) {
            foreach (var ct in t.GenericTypeArguments) {
                if (ct._HasClassMember()) return true;
            }
        }
        return false;
    }


    /**************************************************************************************************/
    // Get 系列
    /**************************************************************************************************/

    /// <summary>
    /// 获取 Attribute 之 TypeId 值. 未找到将返回 null
    /// </summary>
    public static ushort? _GetTypeId(this ICustomAttributeProvider t) {
        foreach (var a in t.GetCustomAttributes(false)) {
            if (a is TemplateLibrary.TypeId)
                return ((TemplateLibrary.TypeId)a).value;
        }
        return null;
    }

    /// <summary>
    /// 获取 Attribute 之 Desc 注释. 未找到将返回 ""
    /// </summary>
    public static string _GetDesc(this ICustomAttributeProvider t) {
        foreach (var a in t.GetCustomAttributes(false)) {
            if (a is TemplateLibrary.Desc)
                return ((TemplateLibrary.Desc)a).value;
        }
        return "";
    }

    /// <summary>
    /// 创建指定类型的实例( 通常为方便获取默认值 )
    /// </summary>
    public static object _GetInstance(this Type t) {
        return cfg.asm.CreateInstance(t.FullName);
    }

    /// <summary>
    /// 获取 Attribute 之 Limit 值. 未找到将返回 0
    /// </summary>
    public static int _GetLimit(this ICustomAttributeProvider t) {
        foreach (var a in t.GetCustomAttributes(false)) {
            if (a is TemplateLibrary.Limit)
                return ((TemplateLibrary.Limit)a).value;
        }
        return 0;
    }

    /// <summary>
    /// 获取 Attribute 之 Limit 值的集合. 未找到将返回 0 长度集合
    /// </summary>
    public static List<int> _GetLimits(this ICustomAttributeProvider t) {
        var rtv = new List<int>();
        foreach (var a in t.GetCustomAttributes(false)) {
            if (a is TemplateLibrary.Limit) {
                rtv.Add(((TemplateLibrary.Limit)a).value);
            }
        }
        return rtv;
    }

    /// <summary>
    /// 获取 class 的成员列表( 含 const )
    /// </summary>
    public static List<FieldInfo> _GetFieldsConsts(this Type t) {
        return t.GetFields(BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Static | BindingFlags.Instance).ToList();
    }

    /// <summary>
    /// 获取 class 的成员列表( 不含 const )
    /// </summary>
    public static List<FieldInfo> _GetFields(this Type t) {
        return t.GetFields(BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance).ToList();
    }

    /// <summary>
    /// 获取 class 的附加有指定 Attribute 的成员列表( 不含 const )
    /// </summary>
    public static List<FieldInfo> _GetFields<T>(this Type t) {
        return t.GetFields(BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance).Where(f => f._Has<T>()).ToList();
    }

    /// <summary>
    /// 获取类型的成员函数列表
    /// </summary>
    public static List<MethodInfo> _GetMethods(this Type t) {
        return t.GetMethods(BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance).Where(
            m => m.Name != "ToString" &&
            m.Name != "Equals" &&
            m.Name != "GetHashCode" &&
            m.Name != "GetType" &&
            m.Name != "Finalize" &&
            m.Name != "MemberwiseClone"
            ).ToList();
    }

    /// <summary>
    /// 获取类型( 特指interface )的 property 列表
    /// </summary>
    public static List<PropertyInfo> _GetProperties(this Type t) {
        return t.GetProperties(BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance).ToList();
    }

    /// <summary>
    /// 获取 t( enum ) 的成员列表
    /// </summary>
    public static List<FieldInfo> _GetEnumFields(this Type t) {
        return t.GetFields(BindingFlags.Static | BindingFlags.Public).ToList();
    }

    /// <summary>
    /// 获取枚举项 f 的数字值
    /// </summary>
    public static string _GetEnumValue(this FieldInfo f, Type e) {
        return f.GetValue(null)._GetEnumInteger(e);
    }

    /// <summary>
    /// 将枚举转为数字值
    /// </summary>
    public static string _GetEnumInteger(this object enumValue, Type e) {
        switch (e.GetEnumUnderlyingType().Name) {
            case "Byte":
                return Convert.ToByte(enumValue).ToString();
            case "SByte":
                return Convert.ToSByte(enumValue).ToString();
            case "UInt16":
                return Convert.ToUInt16(enumValue).ToString();
            case "Int16":
                return Convert.ToInt16(enumValue).ToString();
            case "UInt32":
                return Convert.ToUInt32(enumValue).ToString();
            case "Int32":
                return Convert.ToInt32(enumValue).ToString();
            case "UInt64":
                return Convert.ToUInt64(enumValue).ToString();
            case "Int64":
                return Convert.ToInt64(enumValue).ToString();
        }
        throw new Exception("unknown Underlying Type");
    }


    /**************************************************************************************************/
    // 其他
    /**************************************************************************************************/

    /// <summary>
    /// 递归获取 type 所有子 type( parent, fields, generic T... 都算 ) 填充到 types
    /// </summary>
    public static void FillChildStructTypes(this Type t, List<Type> types) {
        if (t.IsGenericType) {
            foreach (var ct in t.GetGenericArguments()) {
                FillChildStructTypes(ct, types);
            }
        }
        else {
            if (t._HasBaseType()) {
                FillChildStructTypes(t.BaseType, types);
            }
            if (t._IsClass() || t._IsStruct()) {
                if (types.Contains(t)) return;
                types.Add(t);
                foreach (var f in t._GetFields()) {
                    FillChildStructTypes(f.FieldType, types);
                }
            }
        }
    }

    public class TypeCount : IComparable<TypeCount> {
        public Type type;
        public int count;

        public int CompareTo(TypeCount other) {
            return -this.count.CompareTo(other.count);  // 从大到小倒排
        }
    }

    /// <summary>
    /// 根据继承 & 成员引用关系排序, 被引用多的在前( 便于 C++ 生成 )
    /// </summary>
    public static List<Type> _SortByInheritRelation(this List<Type> ts) {

        var typeCounts = new Dictionary<Type, int>();
        foreach (var t in ts) {
            typeCounts.Add(t, 0);
        }

        foreach (var t in ts) {
            var types = new List<Type>();
            FillChildStructTypes(t, types);
            foreach (var type in types) {
                typeCounts[type]++;
            }
        }

        var list = new List<TypeCount>();
        foreach (var t in ts) {
            list.Add(new TypeCount { type = t, count = typeCounts[t] });
        }
        list.Sort();

        ts.Clear();
        ts.AddRange(list.Select(a => a.type));
        return ts;
    }

    class TypeComparer : Comparer<Type> {
        public override int Compare(Type x, Type y) {
            return x.FullName.CompareTo(y.FullName);
        }
    }

    /// <summary>
    /// 根据 FullName 排序
    /// </summary>
    public static List<Type> _SortByFullName(this List<Type> ts) {
        ts.Sort(new TypeComparer());
        return ts;
    }

}

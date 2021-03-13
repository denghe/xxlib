// 全是 Attribute
namespace TemplateLibrary {

    // 语言定位：C++ 通信 + 建模( 支持递归强弱引用 ), C# Lua 仅通信( 只支持基础结构体组合 & 继承 )

    /// <summary>
    /// 对应 c++ std::optional, c# Nullable<>, lua variable
    /// 考虑到 c# string & class & lua table 默认语言级别可空，和 c++ 默认值类型冲突
    /// 如果 不加 Nullable, 则 c# lua 不可传空( 发送时检查 ), C++ 就是直接的值类型
    /// 如果 加了 Nullable, 则 c# lua 可传空( 发送时不检查 ), C++ 包裹 std::optional
    /// </summary>
    public class Nullable<T> { }

    /// <summary>
    /// 对应 c++ std::vector, c# List, lua table
    /// 如果不套 Nullable, 则 C# lua 不可传空( 发送时检查 )
    /// </summary>
    public class List<T> { }

    /// <summary>
    /// 对应 c++ std::weak_ptr, c# ???, lua ???
    /// 暂时不支持 c#, lua, 生成 c# lua 时如果检测到，就直接报错
    /// </summary>
    public class Weak<T> { }

    /// <summary>
    /// 对应 c++ std::shared_ptr, c# Shared/Property, lua ???
    /// 暂时不支持 c#, lua, 生成 c# lua 时如果检测到，就直接报错
    /// </summary>
    public class Shared<T> { }

    /// <summary>
    /// 对应 c++ std::map, c# Dict[ionary]( 无法保证原始顺序 ). lua table( 无法保证原始顺序 )
    /// </summary>
    public class Dict<K, V> { }

    /// <summary>
    /// 对应 c++ std::pair, c# 模拟. lua table 模拟
    /// </summary>
    public class Pair<K, V> { }

    /// <summary>
    /// 对应 c++ std::tuple, c# Tuple. lua table 模拟
    /// </summary>
    public class Tuple<T1> { }
    public class Tuple<T1, T2> { }
    public class Tuple<T1, T2, T3> { }
    public class Tuple<T1, T2, T3, T4> { }
    public class Tuple<T1, T2, T3, T4, T5> { }
    public class Tuple<T1, T2, T3, T4, T5, T6> { }
    public class Tuple<T1, T2, T3, T4, T5, T6, T7> { }


    /// <summary>
    /// 用来做类型到 typeId 的固定映射生成
    /// </summary>
    [System.AttributeUsage(System.AttributeTargets.Class | System.AttributeTargets.Struct)]
    public class TypeId : System.Attribute {
        public TypeId(ushort value) {
            this.value = value;
        }
        public ushort value;
    }

    /// <summary>
    /// 标记一个类能简单向下兼容( 不可修改 顺序 & 数据类型 & 默认值, 变量名可改, 不可删除，只能在最后追加, 且须避免老 Weak 引用到追加成员. 老 Shared 引用到新增类 也是无法识别的 )
    /// 两种情况：读不完，读不够
    /// </summary>
    [System.AttributeUsage(System.AttributeTargets.Class | System.AttributeTargets.Struct)]
    public class Compatible : System.Attribute {
    }

    /// <summary>
    /// 备注。可用于类/枚举/函数 及其 成员
    /// </summary>
    public class Desc : System.Attribute {
        public Desc(string v) { value = v; }
        public string value;
    }

    /// <summary>
    /// 外部扩展。命名空间根据类所在实际命名空间获取，去除根模板名。参数如果传 false 则表示该类不支持序列化，无法用于收发
    /// </summary>
    [System.AttributeUsage(System.AttributeTargets.Enum | System.AttributeTargets.Class | System.AttributeTargets.Struct)]
    public class External : System.Attribute {
    }



    /// <summary>
    /// C++ only
    /// 标记一个类需要抠洞在声明部分嵌入 模板名_类名.inc ( 在成员前面 )
    /// </summary>
    [System.AttributeUsage(System.AttributeTargets.Class | System.AttributeTargets.Struct)]
    public class Include : System.Attribute {
    }

    /// <summary>
    /// C++ only
    /// 标记一个类需要抠洞在声明部分嵌入 模板名_类名_.inc ( 在成员后面 )
    /// </summary>
    [System.AttributeUsage(System.AttributeTargets.Class | System.AttributeTargets.Struct)]
    public class Include_ : System.Attribute {
    }



    /****************************************************************************************/
    // 下面的东西用不用看心情

    /// <summary>
    /// C# only
    /// 标记一个 class 生成时走 struct 规则( 值类型继承 ), 以突破 c# 写模板的限制
    /// </summary>
    [System.AttributeUsage(System.AttributeTargets.Class)]
    public class Struct : System.Attribute { }


    /// <summary>
    /// 针对最外层级的 List, Data, string 做最大长度保护限制
    /// 如果是类似 List List... 多层需要限制的情况, 就写多个 Limit, 有几层写几个
    /// </summary>
    [System.AttributeUsage(System.AttributeTargets.Field | System.AttributeTargets.ReturnValue, AllowMultiple = true)]
    public class Limit : System.Attribute {
        public Limit(int value) {
            this.value = value;
        }
        public int value;
    }
}

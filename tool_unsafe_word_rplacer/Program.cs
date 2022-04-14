using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Threading.Tasks;
using System.Windows.Forms;


namespace UWR
{
    [DataContract]
    public class Config
    {
        [DataMember]
        public List<string> dirs = new List<string>();

        [DataMember]
        public List<string> exts = new List<string>();

        [DataMember]
        public List<string> ignores = new List<string>();

        [DataMember]
        public List<string> safes = new List<string>();

        [DataMember]
        public List<FromTo> replaces = new List<FromTo>();


        /// <summary>
        /// 用来临时记录文件名列表
        /// </summary>
        public List<string> files = new List<string>();
        /// <summary>
        /// 用来记录所有要替换的词. Dictionary(词, Tuple(替换为, Dictionary(文件名, 行号)))
        /// </summary>
        public Dictionary<string, Tuple<string, Dictionary<string, HashSet<int>>>> words = new Dictionary<string, Tuple<string, Dictionary<string, HashSet<int>>>>();
    }

    [DataContract]
    public class FromTo
    {
        [DataMember]
        public string from = "";

        [DataMember]
        public string to = "";
    }
}


namespace UnsafeWordReplacer
{
    /*

    输入文件路径列表

读出所有文本，按 非空格 或 非数字 非字母 非下划线 切割成 短字符串， CatchFish Fish1 split(' ')
去重后，再去每个文件的原始内容 找到 位置

界面大约：

+ word1(有几处, 存在与几个文件 )
   L--  replace?[yes / no] file 1 
   +--  replace?[yes / no] file 2
+ word1
   L-- file 1
   +-- file 2


file 1
alksdjfsadhkf dshkfj hsdkjf hsakf hlfdj skldf jsd
alksdjfsadhkf dshkfj hsdkjf hsakf hlfdj skldf jsd
alksdjfsadhkf dshkfj hsdkjf hsakf hlfdj skldf jsd
alksdjfsadhkf dshkfj hsdkjf hsakf hlfdj skldf jsd
alksdjfsadhkf dsh !!!!!!!!!!! hsakf hlfdj skldf jsd
alksdjfsadhkf dshkfj hsdkjf hsakf hlfdj skldf jsd
alksdjfsadhkf dshkfj hsdkjf hsakf hlfdj skldf jsd
alksdjfsadhkf dshkfj hsdkjf hsakf hlfdj skldf jsd
alksdjfsadhkf dshkfj hsdkjf hsakf hlfdj skldf jsd
alksdjfsadhkf dshkfj hsdkjf hsakf hlfdj skldf jsd

alksdjfsadhkf dshkfj hsdkjf hsakf hlfdj skld!!!!!!!hlfdj skldf jsd
alksdjfsadhkf dshkfj hsdkjf hsakf hlfdj skldf jsd


file2
alksdjfsadhkf dshkfj hsdkjf hsakf hlfdj skldf jsd
alksdjfsadhkf dshkfj hs!!!!!!!!!akf hlfdj skldf jsd
alksdjfsadhkf dshkfj hsdkjf hsakf hlfdj skldf jsd


    */


    internal static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new Form1());
        }
    }
}

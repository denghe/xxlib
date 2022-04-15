using System;
using System.Collections.Generic;
using System.Data;
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
        public HashSet<string> dirs = new HashSet<string>();

        [DataMember]
        public HashSet<string> exts = new HashSet<string>();

        [DataMember]
        public HashSet<string> ignores = new HashSet<string>();

        [DataMember]
        public HashSet<string> safes = new HashSet<string>();

        [DataMember]
        public List<FromTo> replaces = new List<FromTo>();
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

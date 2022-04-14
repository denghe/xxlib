using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace UnsafeWordReplacer
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();

            EnableContent(false);
        }

        private void EnableContent(bool b)
        {
            mExport.Enabled = b;
            mRandomAll.Enabled = b;
            mSave.Enabled = b;
            tvWords.Enabled = b;
            tbReplaceTo.Enabled = b;
            bRestore.Enabled = b;
        }

        private void Form1_Load(object sender, EventArgs e)
        {
        }

        private void tvWords_AfterSelect(object sender, TreeViewEventArgs e)
        {
            tbPreview.Text = e.Node.ToString() + " " + e.Action.ToString();

            //var p = new Person();
            //p.age = 123;
            //p.name = "askdfhlk";

            //var stream1 = new MemoryStream();
            //var ser = new DataContractJsonSerializer(typeof(Person));
            //ser.WriteObject(stream1, p);

            //stream1.Position = 0;
            //var sr = new StreamReader(stream1);
            //MessageBox.Show(sr.ReadToEnd());

            //byte[] json = stream1.ToArray();
            //MessageBox.Show(Encoding.UTF8.GetString(json, 0, json.Length));
            ////File.WriteAllBytes()

            //stream1.Position = 0;
            //var p2 = (Person)ser.ReadObject(stream1);
        }

        private void tbReplaceTo_TextChanged(object sender, EventArgs e)
        {

        }

        private void mImport_Click(object sender, EventArgs e)
        {
            if (openFileDialog1.ShowDialog() == DialogResult.OK)
            {
                //openFileDialog1.FileName
            }

        }

        private void mExport_Click(object sender, EventArgs e)
        {
            if (saveFileDialog1.ShowDialog() == DialogResult.OK)
            {
                MessageBox.Show(saveFileDialog1.FileName);
                //saveFileDialog1.FileName
            }
        }

        private void mRandomAll_Click(object sender, EventArgs e)
        {

        }

        private void mSave_Click(object sender, EventArgs e)
        {

        }

        private void mQuit_Click(object sender, EventArgs e)
        {

        }

        private void mAbout_Click(object sender, EventArgs e)
        {

        }

        private void tvWords_KeyPress(object sender, KeyPressEventArgs e)
        {
            // todo: 如果按的是空格，就将当前选中项 移到 safe words. 按 R 就随机
        }

        /// <summary>
        /// 将 content 按 \n 切割 并 trim, 去重后塞入 tar. 塞前 clear
        /// </summary>
        /// <param name="tar">待填充容器</param>
        /// <param name="content">待处理内容</param>
        /// <returns>塞了多少个</returns>
        private int FillStrings(List<string> tar, string content)
        {
            var hss = new HashSet<string>();
            var ss = content.Trim().Split(new char[] { '\n' }, StringSplitOptions.RemoveEmptyEntries);
            foreach (var s in ss)
            {
                var tmp = s.Trim(' ', '\t');
                if (tmp.Length < 2) continue;
                hss.Add(tmp);
            }
            tar.Clear();
            tar.AddRange(hss);
            return tar.Count;
        }

        private void mScan_Click(object sender, EventArgs e)
        {
            // 禁用内容相关控件
            EnableContent(false);

            // 导入忽略列表
            FillStrings(cfg.ignores, tbIgnores.Text);
            for (int i = 0; i < cfg.ignores.Count; i++)
            {
                cfg.ignores[i] = cfg.ignores[i].Replace("\\", "/");
            }
            // 回写控件
            tbIgnores.Text = String.Join("\n", cfg.ignores);

            // 导入扩展名列表
            if (FillStrings(cfg.exts, tbExts.Text) < 1)
            {
                MessageBox.Show("please input multiline .exts");
                return;
            }
            // 扩展名格式检查
            foreach (var s in cfg.exts)
            {
                if (!s.StartsWith("."))
                {
                    MessageBox.Show("invalid file ext: " + s);
                    return;
                }
            }
            // 回写控件
            tbExts.Text = String.Join("\n", cfg.exts);


            // 导入目录列表
            if (FillStrings(cfg.dirs, tbDirs.Text) < 1)
            {
                MessageBox.Show("please input multiline dirs");
                return;
            }


            // 预处理下
            for (int i = 0; i < cfg.dirs.Count; i++)
            {
                cfg.dirs[i] = cfg.dirs[i].Replace("\\\\", "/").Replace("\\", "/");
                if (cfg.dirs[i].EndsWith("/"))
                {
                    cfg.dirs[i] = cfg.dirs[i].Substring(0, cfg.dirs[i].Length - 1);
                }
            }

            cfg.files.Clear();

            // 开始扫目录 & 文件

            // 去重用
            var dirSet = new HashSet<string>();

            // 递归用
            var dirs = new List<string>();

            // 扫原始列表
            foreach (var s in cfg.dirs)
            {
                var dir = s;
                bool recursive = false;

                if (dir.EndsWith("*"))
                {
                    var subs = dir.Substring(dir.Length - 2);
                    if (subs != "/*" && subs != "\\*")
                    {
                        MessageBox.Show("invalid file dir: " + s);
                        return;
                    }
                    dir = dir.Substring(0, dir.Length - 2);
                    recursive = true;
                }

                if (!Directory.Exists(dir))
                {
                    MessageBox.Show("not found file dir: " + s);
                    return;
                }

                if (IsIgnored(dir)) continue;
                if (dirSet.Contains(dir)) continue;
                dirSet.Add(dir);

                if (recursive)
                {
                    dirs.Add(dir);
                }

                FillFiles(dir);
            }

            var idx = 0;
            while (dirs.Count > idx)
            {
                FillFiles(dirs[idx]);
                foreach (var d in Directory.EnumerateDirectories(dirs[idx], "*", SearchOption.TopDirectoryOnly))
                {
                    var dir = d.Replace("\\\\", "/").Replace("\\", "/");

                    if (IsIgnored(dir)) continue;
                    if (dirSet.Contains(dir)) continue;
                    dirSet.Add(dir);
                    dirs.Add(dir);
                }
                ++idx;
            }

            // 开始搞文件
            // 按 \n 切割成行，trim 掉前后 空格\t\r\n, 再 扫出 连续 2 个以上长度字符在 大小写a-z, 0-9, _ 的
            // 需要记录原始行号

            // 数据格式 Dict<word, Dict<file name, List<line number>>>
            cfg.words.Clear();
            foreach (var f in cfg.files)
            {
                var ss = File.ReadAllLines(f, Encoding.UTF8);
                for (int i = 0; i < ss.Length; i++)
                {
                    var line = ss[i].Trim(' ', '\t', '\r', '\n');
                    int x = -1;
                    for (int j = 0; j < line.Length; j++)
                    {
                        var c = line[j];
                        if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_')
                        {
                            if (x == -1)
                            {
                                x = j;
                            }
                        }
                        else if (x != -1)
                        {
                            if (j - x > 1)
                            {
                                var word = line.Substring(x, j - x);
                                Tuple<string, Dictionary<string, HashSet<int>>> fileLineNumbers;
                                HashSet<int> lineNumbers;
                                if (!cfg.words.TryGetValue(word, out fileLineNumbers))
                                {
                                    fileLineNumbers = new Tuple<string, Dictionary<string, HashSet<int>>>(word, new Dictionary<string, HashSet<int>>());
                                    cfg.words.Add(word, fileLineNumbers);
                                }
                                if (!fileLineNumbers.Item2.TryGetValue(f, out lineNumbers))
                                {
                                    lineNumbers = new HashSet<int>();
                                    fileLineNumbers.Item2.Add(f, lineNumbers);
                                }
                                lineNumbers.Add(i);
                            }
                            x = -1;
                        }
                    }
                }
            }


            tvWords.BeginUpdate();
            tvWords.SelectedNode = null;
            tvWords.Nodes.Clear();
            foreach (var word in cfg.words)
            {
                cfg.replaces.Add(new UWR.FromTo { from = word.Key, to = word.Value.Item1 });
                tvWords.Nodes.Add(word.Key);
            }
            tvWords.EndUpdate();

            EnableContent(cfg.words.Count > 0);
        }


        private void FillFiles(string dir)
        {
            var fs = Directory.EnumerateFiles(dir, "*", SearchOption.TopDirectoryOnly);
            foreach (var f in fs)
            {
                var i = f.LastIndexOf('.');
                var ext = f;
                if (i != -1)
                {
                    ext = f.Substring(i, f.Length - i);
                }
                if (cfg.exts.Contains(ext))
                {
                    cfg.files.Add(f);
                }
            }
        }

        private bool IsIgnored(string path)
        {
            var i = path.LastIndexOf('/');
            var s = path;
            if (i != -1)
            {
                s = path.Substring(i + 1, path.Length - i - 1);
            }
            return cfg.ignores.Contains(s);
        }

        private void bRestore_Click(object sender, EventArgs e)
        {

        }

        private UWR.Config cfg = new UWR.Config();
    }
}

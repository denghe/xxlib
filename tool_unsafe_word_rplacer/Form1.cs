using System;
using System.Collections.Generic;
using System.Data;
using System.IO;
using System.Linq;
using System.Runtime.Serialization.Json;
using System.Text;
using System.Windows.Forms;

namespace UnsafeWordReplacer
{
    public partial class Form1 : Form
    {
        private UWR.Config cfg = new UWR.Config();

        /// <summary>
        /// 用来临时记录文件名列表
        /// </summary>
        List<string> files = new List<string>();

        public class ReplaceToFileLineNumber
        {
            public string replaceTo = "";
            public Dictionary<string, HashSet<int>> fileLineNumbers = new Dictionary<string, HashSet<int>>();
        }

        /// <summary>
        /// 用来记录所有要替换的词. Dictionary(词, Tuple(替换为, Dictionary(文件名, 行号)))
        /// </summary>
        Dictionary<string, ReplaceToFileLineNumber> words = new Dictionary<string, ReplaceToFileLineNumber>();

        /// <summary>
        /// 用于 datagrid 数据源 bind 显示
        /// </summary>
        DataTable wordsTable = new DataTable();

        /// <summary>
        /// words -> replaces
        /// </summary>
        public void FillReplacesByWords()
        {
            cfg.replaces.Clear();
            foreach (var o in words)
            {
                cfg.replaces.Add(new UWR.FromTo { from = o.Key, to = o.Value.replaceTo });
            }
        }

        /// <summary>
        /// words -> wordsTable
        /// </summary>
        public void FillWordsTableByWords()
        {
            if (wordsTable.Columns.Count == 0)
            {
                wordsTable.Columns.Add(new DataColumn("From"));
                wordsTable.Columns.Add(new DataColumn("To"));
                wordsTable.Columns.Add(new DataColumn("Nums"));
            }
            wordsTable.Rows.Clear();
            foreach (var o in words)
            {
                wordsTable.Rows.Add(o.Key, o.Value.replaceTo, o.Value.fileLineNumbers.Count);
            }
        }

        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            mScan_Click(null, null);
        }

        private void mExport_Click(object sender, EventArgs e)
        {
            if (saveFileDialog1.ShowDialog() == DialogResult.OK)
            {
                var s = new MemoryStream();
                var ser = new DataContractJsonSerializer(cfg.GetType());
                ser.WriteObject(s, cfg);

                File.WriteAllBytes(saveFileDialog1.FileName, s.ToArray());
            }
        }

        private void mRandomAll_Click(object sender, EventArgs e)
        {
            // todo
        }

        private void mAbout_Click(object sender, EventArgs e)
        {
            // todo
        }

        private void mRandomSelected_Click(object sender, EventArgs e)
        {
            // todo
        }


        private void mImport_Click(object sender, EventArgs e)
        {
            if (openFileDialog1.ShowDialog() == DialogResult.OK)
            {
                var c = LoadConfig(openFileDialog1.FileName);
                if (c == null) return;
                cfg = c;
                // todo: fill words & wordsTable
                words.Clear();
                wordsTable.Clear();
                foreach(var o in cfg.replaces)
                {
                    words.Add(o.from, new ReplaceToFileLineNumber { replaceTo = o.to });
                }
                FillWordsTableByWords();
                UpdateUI();
            }
        }

        private void mMoveAllWordsToSafes_Click(object sender, EventArgs e)
        {
            if (MessageBox.Show("Are you sure you want to add all words to the safe list?", "warning", MessageBoxButtons.YesNo) == DialogResult.Yes)
            {
                foreach (var o in words)
                {
                    cfg.safes.Add(o.Key);
                }
                words.Clear();
                FillReplacesByWords();
                FillWordsTableByWords();
                UpdateUI();
            }
        }

        private void mScan_Click(object sender, EventArgs e)
        {
            // 导入忽略列表
            FillStrings(cfg.ignores, tbIgnores.Text);

            // 替换 \ 为 /, 去最后一个 /
            ReplaceSpecialChars(cfg.ignores);

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
            // 替换 \ 为 /, 去最后一个 /
            ReplaceSpecialChars(cfg.dirs);
            // 回写控件
            tbDirs.Text = String.Join("\n", cfg.dirs);


            // 导入安全列表( 借用一下函数 )
            FillStrings(cfg.safes, tbSafes.Text);

            // 进一步切割为单个单词( 按 空格 分开 )
            var safeLines = cfg.safes.ToArray();
            cfg.safes.Clear();
            foreach (var s in safeLines)
            {
                var ss = s.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
                foreach (var o in ss)
                {
                    cfg.safes.Add(o);
                }
            }
            // 回写控件
            tbSafes.Text = String.Join(" ", cfg.safes);

            // 开始扫目录 & 文件
            files.Clear();

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
            //words.Clear();
            foreach (var f in files)
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
                                if (!cfg.safes.Contains(word))
                                {
                                    ReplaceToFileLineNumber fileLineNumbers;
                                    HashSet<int> lineNumbers;
                                    if (!words.TryGetValue(word, out fileLineNumbers))
                                    {
                                        fileLineNumbers = new ReplaceToFileLineNumber();
                                        fileLineNumbers.replaceTo = word;
                                        words.Add(word, fileLineNumbers);
                                    }
                                    if (!fileLineNumbers.fileLineNumbers.TryGetValue(f, out lineNumbers))
                                    {
                                        lineNumbers = new HashSet<int>();
                                        fileLineNumbers.fileLineNumbers.Add(f, lineNumbers);
                                    }
                                    lineNumbers.Add(i);
                                }
                            }
                            x = -1;
                        }
                    }
                }
            }

            // 写入导出 json 要用的字段
            FillReplacesByWords();

            // 填充用于数据 bind 的字段
            FillWordsTableByWords();

            // 绑定显示
            dgWords.DataSource = wordsTable;
        }




        /// <summary>
        /// 将 content 按 \n 切割 并 trim, 去重后塞入 tar. 塞前 clear
        /// </summary>
        /// <param name="tar">待填充容器</param>
        /// <param name="content">待处理内容</param>
        /// <returns>塞了多少个</returns>
        private int FillStrings(HashSet<string> tar, string content)
        {
            tar.Clear();
            var ss = content.Trim().Split(new char[] { '\n' }, StringSplitOptions.RemoveEmptyEntries);
            foreach (var s in ss)
            {
                var tmp = s.Trim(' ', '\t');
                if (tmp.Length < 2) continue;
                tar.Add(tmp);
            }
            return tar.Count;
        }

        /// <summary>
        /// 将指定目录下所有文件( 表层 ) path 塞到 cfg.files
        /// </summary>
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
                    files.Add(f);
                }
            }
        }

        /// <summary>
        /// 判断一个路径是否已被忽略
        /// </summary>
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

        /// <summary>
        /// 替换 \ 为 /, 去最后一个 /
        /// </summary>
        /// <param name="tar">待处理容器</param>
        private void ReplaceSpecialChars(HashSet<string> tar)
        {
            var arr = tar.Select(s => s.Replace("\\\\", "/").Replace("\\", "/")).ToArray();
            tar.Clear();
            foreach (var s in arr)
            {
                tar.Add(s.EndsWith("/") ? s.Substring(0, s.Length - 1) : s);
            }
        }


        /// <summary>
        /// 从一个 json 文件加载 WUR.Config 并返回
        /// </summary>
        private UWR.Config LoadConfig(string fileName)
        {
            try
            {
                var bytes = File.ReadAllBytes(openFileDialog1.FileName);
                var ms = new MemoryStream(bytes);
                var ser = new DataContractJsonSerializer(typeof(UWR.Config));
                return (UWR.Config)ser.ReadObject(ms);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
                return null;
            }
        }

        /// <summary>
        /// 从 config 同步显示到 UI
        /// </summary>
        private void UpdateUI()
        {
            tbDirs.Text = String.Join("\n", cfg.dirs);
            tbExts.Text = String.Join("\n", cfg.exts);
            tbIgnores.Text = String.Join("\n", cfg.ignores);
            tbSafes.Text = String.Join(" ", cfg.safes);
            dgWords.DataSource = wordsTable;
            WordSelected(null);
        }

        /// <summary>
        /// word 被选中后，更新 UI
        /// </summary>
        private void WordSelected(string word)
        {
            if (word == null)
            {
                tbPreview.Text = "";
            }
            else
            {
                // todo: 加载所有 含有 word 的文件内容，填充到 tbPreview, 每个文件生成 彩色头部，正文 word 上下行数保留 10 行, word 染色
                var info = words[word];
                foreach(var kv in info.fileLineNumbers)
                {
                    var lines = File.ReadLines(kv.Key);
                    tbPreview.Text = "";
                    // todo
                    // 追加 文件名
                    // 每行前面加行号
                    // word 替换处 染色
                    tbPreview.Text = String.Join("\n", lines);
                }
            }
        }

        private void dgWords_RowStateChanged(object sender, DataGridViewRowStateChangedEventArgs e)
        {
            if(e.StateChanged == DataGridViewElementStates.Selected)
            {
                WordSelected((string)e.Row.Cells[0].Value);
            }
        }

        private void dgWords_CellStateChanged(object sender, DataGridViewCellStateChangedEventArgs e)
        {
            if (e.StateChanged == DataGridViewElementStates.Selected)
            {
                WordSelected((string)e.Cell.OwningRow.Cells[0].Value);
            }
        }

        private void dgWords_CellValueChanged(object sender, DataGridViewCellEventArgs e)
        {
            if (e.RowIndex < 0 || e.ColumnIndex != 1) return;
            var word = (string)dgWords.Rows[e.RowIndex].Cells[0].Value;
            var newValue = (string)dgWords.Rows[e.RowIndex].Cells[e.ColumnIndex].Value;
            // todo: 重复检查。如果已存在 就要 恢复回去
            words[word].replaceTo = newValue;
            FillReplacesByWords();
            WordSelected(word);
        }
    }
}

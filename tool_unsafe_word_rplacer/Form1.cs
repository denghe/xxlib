using System;
using System.Collections.Generic;
using System.Data;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Text;
using System.Windows.Forms;

namespace UnsafeWordReplacer
{
    public partial class Form1 : Form
    {
        private Config cfg = new Config();

        /// <summary>
        /// 用来临时记录文件名列表
        /// </summary>
        List<string> files = new List<string>();

        public class ReplaceToFileLineNumberFromTo
        {
            public string replaceTo = "";
            public List<string> resFilePaths = new List<string>();
            public Dictionary<string, Dictionary<int, List<int>>> fileLineNumbers = new Dictionary<string, Dictionary<int, List<int>>>();
        }

        /// <summary>
        /// 用来记录所有要替换的词. Dictionary(词, Tuple(替换为, Dictionary(文件名, 行号)))
        /// </summary>
        Dictionary<string, ReplaceToFileLineNumberFromTo> words = new Dictionary<string, ReplaceToFileLineNumberFromTo>();

        /// <summary>
        /// 用于 datagrid 数据源 bind 显示
        /// </summary>
        DataTable wordsTable = new DataTable();
        DataTable emptyWordsTable = new DataTable();

        /// <summary>
        /// replaceTo 的集合。用于快速判断是否重复
        /// </summary>
        public HashSet<string> replaces = new HashSet<string>();

        /// <summary>
        /// 随机生成 replaceTo 用
        /// </summary>
        public Random rnd = new Random(DateTime.Now.Millisecond);

        /// <summary>
        /// 随机生成 replaceTo 的字符范围. 首字母不含数字
        /// </summary>
        public string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789";

        /// <summary>
        /// 记录 dgWord 最后焦点行号 以便响应 Random 菜单
        /// </summary>
        public int lastWordIndex = -1;

        /// <summary>
        /// warnings
        /// </summary>
        public List<string> warnings = new List<string>();


        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            emptyWordsTable.Columns.Add(new DataColumn("From"));
            emptyWordsTable.Columns.Add(new DataColumn("To"));
            emptyWordsTable.Columns.Add(new DataColumn("Nums"));

            wordsTable.Columns.Add(new DataColumn("From"));
            wordsTable.Columns.Add(new DataColumn("To"));
            wordsTable.Columns.Add(new DataColumn("Nums"));

            dgWords.DataSource = emptyWordsTable;
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
            replaces.Clear();
            cfg.replaces.Clear();
            foreach (var kv in words)
            {
                kv.Value.replaceTo = GenRandomReplaceWord(kv.Key, false);
                var ok = replaces.Add(kv.Value.replaceTo);
                Debug.Assert(ok);
                cfg.replaces.Add(new FromTo { from = kv.Key, to = kv.Value.replaceTo });
            }
            FillWordsTableByWords();
            UpdateUI();
        }

        private void mAbout_Click(object sender, EventArgs e)
        {
            MessageBox.Show("Any question, please contact author.", "info", MessageBoxButtons.OK);
        }

        private void mRandomSelected_Click(object sender, EventArgs e)
        {
            var cs = dgWords.Rows[lastWordIndex].Cells;
            var word = cs[0].Value.ToString();
            var v = words[word];
            v.replaceTo = GenRandomReplaceWord(word, false);
            FillReplacesByWords();
            wordsTable.Rows[lastWordIndex][1] = v.replaceTo;
            WordSelected(word);
        }


        private void mImport_Click(object sender, EventArgs e)
        {
            if (openFileDialog1.ShowDialog() == DialogResult.OK)
            {
                var c = LoadConfig(openFileDialog1.FileName);
                if (c == null) return;
                cfg = c;
                words.Clear();
                wordsTable.Clear();
                foreach (var o in cfg.replaces)
                {
                    words.Add(o.from, new ReplaceToFileLineNumberFromTo { replaceTo = o.to });
                }
                FillWordsTableByWords();
                UpdateUI();
            }
        }

        private void mMoveAllWordsToSafes_Click(object sender, EventArgs e)
        {
            bool hasRes = false;
            foreach (var v in words.Values)
            {
                if (v.resFilePaths.Count > 0)
                {
                    hasRes = true;
                    break;
                }
            }
            DialogResult r = DialogResult.No;
            if (hasRes)
            {
                r = MessageBox.Show("Contain res file name & path words ?", "warning", MessageBoxButtons.YesNo);
            }
            if (r == DialogResult.No)
            {
                foreach (var k in words.Keys.ToArray())
                {
                    if (words[k].resFilePaths.Count > 0) continue;
                    cfg.safes.Add(k);
                    words.Remove(k);
                }
            }
            else
            {
                foreach (var o in words)
                {
                    cfg.safes.Add(o.Key);
                }
                words.Clear();
            }
            FillReplacesByWords();
            FillWordsTableByWords();
            UpdateUI();
        }

        private string[] GetSelectedWords()
        {
            var src = dgWords.SelectedRows.Count;
            var scc = dgWords.SelectedCells.Count;
            if (src + scc == 0) return null;

            var ws = new HashSet<string>();
            if (src > 0)
            {
                foreach (DataGridViewRow row in dgWords.SelectedRows)
                {
                    ws.Add((string)row.Cells[0].Value);
                }
            }
            else
            {
                foreach (DataGridViewCell cell in dgWords.SelectedCells)
                {
                    ws.Add((string)dgWords.Rows[cell.RowIndex].Cells[0].Value);
                }
            }

            return ws.Where(o => o != null).ToArray();
        }

        private void mMoveSelectedWordToSafes_Click(object sender, EventArgs e)
        {
            var ss = GetSelectedWords();
            if (ss == null) return;

            foreach (var s in ss)
            {
                var r = cfg.safes.Add(s);
                Debug.Assert(r);

                words.Remove(s);
            }
            FillReplacesByWords();
            FillWordsTableByWords();
            UpdateUI();
        }


        private void mDeleteSelectedWords_Click(object sender, EventArgs e)
        {
            var ss = GetSelectedWords();
            if (ss == null) return;

            foreach (var s in ss)
            {
                words.Remove(s);
            }
            FillReplacesByWords();
            FillWordsTableByWords();
            UpdateUI();
        }

        private void mScan_Click(object sender, EventArgs e)
        {
            // clean up
            foreach (var o in words)
            {
                o.Value.resFilePaths.Clear();
                o.Value.fileLineNumbers.Clear();
            }

            // 导入忽略列表
            FillStrings(cfg.ignores, tbIgnores.Text);

            // 替换 \ 为 /, 去最后一个 /
            ReplaceSpecialChars(cfg.ignores);

            // 回写控件
            tbIgnores.Text = String.Join("\n", cfg.ignores);

            // 导入扩展名列表
            if (FillStrings(cfg.exts, tbExts.Text) < 1)
            {
                MessageBox.Show("please input multiline code file .exts");
                return;
            }
            // 扩展名格式检查
            foreach (var s in cfg.exts)
            {
                if (!s.StartsWith("."))
                {
                    MessageBox.Show("invalid code file ext: " + s);
                    return;
                }
            }
            // 回写控件
            tbExts.Text = String.Join("\n", cfg.exts);

            // 导入资源扩展名列表
            if (FillStrings(cfg.resExts, tbResExts.Text) < 1)
            {
                MessageBox.Show("please input multiline res file .exts");
                return;
            }
            // 扩展名格式检查
            foreach (var s in cfg.resExts)
            {
                if (!s.StartsWith("."))
                {
                    MessageBox.Show("invalid res file ext: " + s);
                    return;
                }
            }
            // 回写控件
            tbResExts.Text = String.Join("\n", cfg.resExts);


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
            var dirIsRess = new List<bool>();

            // 扫原始列表
            foreach (var s in cfg.dirs)
            {
                if (s.StartsWith("#")) continue;

                var dir = s;
                bool recursive = false;
                bool isRes = false;

                if (dir.EndsWith("*"))
                {
                    var subs = dir.Substring(dir.Length - 2);
                    if (subs != "\\*")
                    {
                        MessageBox.Show("invalid file dir: " + s);
                        return;
                    }
                    dir = dir.Substring(0, dir.Length - 2);
                    recursive = true;
                }

                if (dir.StartsWith("!"))
                {
                    dir = dir.Substring(1);
                    isRes = true;
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
                    dirIsRess.Add(isRes);
                }
                else
                {
                    if (isRes)
                    {
                        FillRess(dir);
                    }
                    else
                    {
                        FillFiles(dir);
                    }
                }
            }

            var idx = 0;
            while (dirs.Count > idx)
            {
                var dd = dirs[idx];
                bool isRes = dirIsRess[idx];

                if (isRes)
                {
                    FillRess(dd);
                }
                else
                {
                    FillFiles(dd);
                }

                foreach (var d in Directory.EnumerateDirectories(dd, "*", SearchOption.TopDirectoryOnly))
                {
                    var dir = d.Replace("/", "\\").Replace("\\\\", "\\");

                    if (IsIgnored(dir)) continue;
                    if (dirSet.Contains(dir)) continue;
                    dirSet.Add(dir);

                    dirs.Add(dir);
                    dirIsRess.Add(isRes);
                }
                ++idx;
            }

            // 开始搞文件
            // 按 \n 切割成行，trim 掉前后 空格\t\r\n, 再 扫出 连续 2 个以上长度字符在 大小写a-z, 0-9, _ 的
            // 需要记录原始行号


            foreach (var o in words.Keys.ToArray())
            {
                if (cfg.safes.Contains(o))
                {
                    words.Remove(o);
                }
            }


            foreach (var f in files)
            {
                FillFileWords(f);
            }

            // 写入导出 json 要用的字段
            FillReplacesByWords();

            // 填充用于数据 bind 的字段
            FillWordsTableByWords();

            // 绑定显示
            dgWords.DataSource = wordsTable;

            // 如果有警告就提示
            if (warnings.Count > 0)
            {
                MessageBox.Show(string.Join("\n", warnings));
                warnings.Clear();
            }
        }


        private void dgWords_CellValueChanged(object sender, DataGridViewCellEventArgs e)
        {
            if (e.RowIndex < 0 || e.ColumnIndex != 1) return;
            var cells = dgWords.Rows[e.RowIndex].Cells;
            var word = (string)cells[0].Value;
            var newValue = (string)cells[e.ColumnIndex].Value;
            if (newValue == word) return;
            // 重复检查。如果已存在 就要 报错 & 恢复回去
            if (words.ContainsKey(newValue))
            {
                MessageBox.Show("Duplicated !!", "warning", MessageBoxButtons.OK);
                cells[e.ColumnIndex].Value = word;
                return;
            }
            // 更新并同步
            words[word].replaceTo = newValue;
            FillReplacesByWords();
            WordSelected(word);
        }

        private void dgWords_RowEnter(object sender, DataGridViewCellEventArgs e)
        {
            if (e.RowIndex < 0)
            {
                WordSelected(null);
                lastWordIndex = -1;
                return;
            }
            lastWordIndex = e.RowIndex;
            var word = (string)dgWords.Rows[e.RowIndex].Cells[0].Value;
            WordSelected(word);
        }

        /// <summary>
        /// 返回 s 是否是个数字
        /// </summary>
        private bool IsNumber(string s)
        {
            if (s.All(o => o >= '0' && o <= '9')) return true;
            s = s.ToLower();
            if (s.StartsWith("0x") || s.StartsWith("0b"))
            {
                s = s.Substring(2);
                if (s.All(o => o >= '0' && o <= '9' || o >= 'a' && o <= 'f')) return true;
            }
            if (s.EndsWith("e") && s.Substring(0, s.Length - 1).All(o => o >= '0' && o <= '9')) return true;
            return false;
        }

        private void FillFileWords(string f)
        {
            var ss = File.ReadAllLines(f, Encoding.UTF8);
            for (int i = 0; i < ss.Length; i++)
            {
                var line = ss[i];//.Trim(' ', '\t', '\r', '\n');
                int x = -1;
                for (int j = 0; j < line.Length; j++)
                {
                    var c = line[j];
                    if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c >= '0' && c <= '9' || c == '_')
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
                            if (!IsNumber(word) && !cfg.safes.Contains(word))
                            {
                                ReplaceToFileLineNumberFromTo fileLineNumbers;
                                Dictionary<int, List<int>> lineNumbers;
                                List<int> idxs;
                                if (!words.TryGetValue(word, out fileLineNumbers))
                                {
                                    fileLineNumbers = new ReplaceToFileLineNumberFromTo();
                                    fileLineNumbers.replaceTo = word;
                                    words.Add(word, fileLineNumbers);
                                }
                                if (!fileLineNumbers.fileLineNumbers.TryGetValue(f, out lineNumbers))
                                {
                                    lineNumbers = new Dictionary<int, List<int>>();
                                    fileLineNumbers.fileLineNumbers.Add(f, lineNumbers);
                                }
                                if (!lineNumbers.TryGetValue(i, out idxs))
                                {
                                    idxs = new List<int>();
                                    lineNumbers.Add(i, idxs);
                                }
                                idxs.Add(x);
                            }
                        }
                        x = -1;
                    }
                }
            }
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
        /// 提取文件扩展名( 倒着找 . 并记录位置, 直到遇到 \ 停止，从最后 . 的位置切割 )
        /// </summary>
        private string GetFileExt(string fn)
        {
            if (fn == null || fn.Length == 0) return "";
            int pos = -1;
            for (int i = fn.Length - 1; i >= 0; i--)
            {
                switch (fn[i])
                {
                    case '.':
                        pos = i;
                        break;
                    case '\\':
                        goto TheEnd;
                    default:
                        continue;
                }
            }
        TheEnd:
            if (pos == -1) return "";
            return fn.Substring(pos);
        }

        /// <summary>
        /// 将指定目录下所有文件( 表层 ) path 塞到 files
        /// </summary>
        private void FillFiles(string dir)
        {
            var fs = Directory.EnumerateFiles(dir, "*", SearchOption.TopDirectoryOnly);
            foreach (var f in fs)
            {
                var s = f.Replace("/", "\\").Replace("\\\\", "\\");
                var ext = GetFileExt(s).ToLower();
                if (cfg.exts.Contains(ext))
                {
                    files.Add(s);
                }
            }
        }

        /// <summary>
        /// 将指定目录下所有 资源文件 文件名( 表层 ) 塞到 words
        /// </summary>
        private void FillRess(string dir)
        {
            var fs = Directory.EnumerateFiles(dir, "*", SearchOption.TopDirectoryOnly).ToArray();
            dir = dir.Replace("/", "\\").Replace("\\\\", "\\");
            foreach (var f in fs)
            {
                var path = f.Replace("/", "\\").Replace("\\\\", "\\");

                // 顺便检查 路径, 文件名 是否符合规范（ 全小写_蛇 )
                var tmp = path.Substring(dir.Length).Replace("\\", "").Replace(".", "");
                if (tmp.Any(o => !(o >= '0' && o <= '9' || o >= 'a' && o <= 'z' || o == '_')))
                {
                    warnings.Add("bad res path( 0-9 a-z _ ): " + path);
                }

                var s = path;
                var ext = GetFileExt(s).ToLower();
                if (cfg.resExts.Contains(ext))
                {
                    s = s.Substring(0, s.Length - ext.Length);
                    var i = s.LastIndexOf('\\');
                    s = s.Substring(i + 1);
                    {
                        var word = s;
                        if (!cfg.safes.Contains(word))
                        {
                            ReplaceToFileLineNumberFromTo fileLineNumbers;
                            if (!words.TryGetValue(word, out fileLineNumbers))
                            {
                                fileLineNumbers = new ReplaceToFileLineNumberFromTo();
                                fileLineNumbers.replaceTo = word;
                                words.Add(word, fileLineNumbers);
                            }
                            fileLineNumbers.resFilePaths.Add(path);
                        }
                    }

                    // 目录名 切割后 纳入 words
                    var subPath = path.Substring(dir.Length, path.Length - dir.Length - s.Length - ext.Length);
                    var ww = subPath.Split(new char[] { '\\', '.' }, StringSplitOptions.RemoveEmptyEntries);
                    if (ww.Length > 0)
                    {
                        foreach (var word in ww)
                        {
                            if (!cfg.safes.Contains(word))
                            {
                                ReplaceToFileLineNumberFromTo fileLineNumbers;
                                if (!words.TryGetValue(word, out fileLineNumbers))
                                {
                                    fileLineNumbers = new ReplaceToFileLineNumberFromTo();
                                    fileLineNumbers.replaceTo = word;
                                    words.Add(word, fileLineNumbers);
                                }
                                fileLineNumbers.resFilePaths.Add(subPath);
                            }
                        }
                    }

                    // 如果该资源文件是代码，就提取内容 words
                    if (cfg.exts.Contains(ext))
                    {
                        FillFileWords(f);
                    }

                }
            }
        }

        /// <summary>
        /// 判断一个路径是否已被忽略
        /// </summary>
        private bool IsIgnored(string path)
        {
            var i = path.LastIndexOf('\\');
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
            var arr = tar.Select(s => s.Replace("/", "\\").Replace("\\\\", "\\")).ToArray();
            tar.Clear();
            foreach (var s in arr)
            {
                tar.Add(s.EndsWith("\\") ? s.Substring(0, s.Length - 1) : s);
            }
        }


        /// <summary>
        /// 从一个 json 文件加载 WUR.Config 并返回
        /// </summary>
        private Config LoadConfig(string fileName)
        {
            try
            {
                var bytes = File.ReadAllBytes(openFileDialog1.FileName);
                var ms = new MemoryStream(bytes);
                var ser = new DataContractJsonSerializer(typeof(Config));
                return (Config)ser.ReadObject(ms);
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
            WordSelected(null);
        }

        /// <summary>
        /// 随机生成一个单词的替换内容。和别的已存在的不重复    // todo: 似乎可根据应用场景替换为 白名单中的等长词汇? 可能需要符合语言语法规则
        /// </summary>
        private string GenRandomReplaceWord(string word, bool searchInSafesFirst)
        {
            if (searchInSafesFirst)
            {
                var oo = from o in cfg.safes where o.Length == word.Length && !replaces.Contains(o) select o;
                var ss = oo.ToArray();
                if (ss.Length > 0)
                {
                    return ss[rnd.Next(ss.Length)];
                }
            }
            var buf = new char[word.Length];
            var s = "";
            do
            {
                for (int i = 0; i < word.Length; i++)
                {
                    buf[i] = chars[rnd.Next(chars.Length - (i == 0 ? 11 : 1))];
                }
                s = new string(buf);
            } while (replaces.Contains(s));
            return s;
        }

        /// <summary>
        /// word 被选中后，更新 UI
        /// </summary>
        private void WordSelected(string word)
        {
            tbPreview.ScrollBars = RichTextBoxScrollBars.None;
            tbPreview.Text = "";
            if (word == null) return;
            // 加载所有 含有 word 的文件内容，填充到 tbPreview, 每个文件生成 彩色头部，正文 word 上下行数保留 10 行, word 染色
            var info = words[word];

            // 如果 resFilePaths 有东西就显示在头部
            if (info.resFilePaths.Count > 0)
            {
                var sb = new StringBuilder();
                foreach (var p in info.resFilePaths)
                {
                    sb.AppendLine(p);
                }
                tbPreview.DeselectAll();
                tbPreview.SelectionColor = System.Drawing.Color.White;
                tbPreview.SelectionBackColor = System.Drawing.Color.Red;
                tbPreview.AppendText(sb.ToString());
            }

            var ns = new HashSet<int>();
            foreach (var kv in info.fileLineNumbers)
            {
                // 读出所有行
                var lines = File.ReadLines(kv.Key).ToArray();

                // 算行号. 保留 上下 各 ? 行. 合并重复的行
                ns.Clear();
                foreach (var n in kv.Value)
                {
                    var begin = n.Key - 8;
                    var end = n.Key + 8;
                    if (begin < 0) begin = 0;
                    if (end >= lines.Length) end = lines.Length - 1;

                    for (int i = begin; i <= end; i++)
                    {
                        ns.Add(i);
                    }
                }

                // 彩色头部, 包含文件路径
                tbPreview.DeselectAll();
                tbPreview.SelectionColor = System.Drawing.Color.Blue;
                tbPreview.SelectionBackColor = System.Drawing.Color.Yellow;
                tbPreview.AppendText($@"{ kv.Key }
");
                // 拼接 显示
                foreach (var n in ns)
                {
                    // 行号染色
                    {
                        var s = (n + 1).ToString();
                        s += new string(' ', 7 - s.Length);
                        tbPreview.SelectionColor = System.Drawing.Color.Blue;
                        tbPreview.AppendText(s);
                    }
                    // 关键字染色 并替换显示
                    // 已知问题：下列代码只替换了当前关键字。如果附近还有别的要替换，并不体现
                    if (kv.Value.ContainsKey(n))
                    {
                        var L = lines[n];
                        var idxs = kv.Value[n];
                        int i = 0;
                        foreach (var idx in idxs)
                        {
                            if (idx > i)
                            {
                                var s = L.Substring(i, idx - i);
                                tbPreview.SelectionColor = System.Drawing.Color.Black;
                                tbPreview.SelectionBackColor = System.Drawing.Color.White;
                                tbPreview.AppendText(s);
                            }
                            tbPreview.SelectionColor = System.Drawing.Color.White;
                            tbPreview.SelectionBackColor = System.Drawing.Color.BlueViolet;
                            tbPreview.AppendText(info.replaceTo);
                            i = idx + word.Length;
                        }
                        tbPreview.SelectionColor = System.Drawing.Color.Black;
                        tbPreview.SelectionBackColor = System.Drawing.Color.White;
                        if (i < L.Length - 1)
                        {
                            tbPreview.AppendText(L.Substring(i));
                        }
                        tbPreview.AppendText("\r\n");
                    }
                    else
                    {
                        tbPreview.SelectionColor = System.Drawing.Color.Black;
                        tbPreview.SelectionBackColor = System.Drawing.Color.White;
                        tbPreview.AppendText(lines[n] + "\r\n");
                    }
                }
            }

            tbPreview.ScrollBars = RichTextBoxScrollBars.Vertical;
        }


        /// <summary>
        /// words -> replaces
        /// </summary>
        public void FillReplacesByWords()
        {
            cfg.replaces.Clear();
            replaces.Clear();
            foreach (var o in words)
            {
                cfg.replaces.Add(new FromTo { from = o.Key, to = o.Value.replaceTo });
                var ok = replaces.Add(o.Value.replaceTo);
                Debug.Assert(ok);
            }
        }

        /// <summary>
        /// words -> wordsTable
        /// </summary>
        public void FillWordsTableByWords()
        {
            var bak = dgWords.FirstDisplayedScrollingRowIndex;
            dgWords.DataSource = emptyWordsTable;
            dgWords.ClearSelection();
            dgWords.CurrentCell = null;

            wordsTable.Rows.Clear();
            foreach (var o in words)
            {
                wordsTable.Rows.Add(o.Key, o.Value.replaceTo, o.Value.fileLineNumbers.Count);
            }

            dgWords.DataSource = wordsTable;
            dgWords.FirstDisplayedScrollingRowIndex = bak;
        }

    }

    [DataContract]
    public class Config
    {
        [DataMember]
        public HashSet<string> dirs = new HashSet<string>();

        [DataMember]
        public HashSet<string> exts = new HashSet<string>();

        [DataMember]
        public HashSet<string> resExts = new HashSet<string>();

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


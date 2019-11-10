/*
    Copyright 2019 php42

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using System.IO;

namespace patcher
{
    public partial class Form1 : Form
    {
        private Process proc_;
        public delegate void do_the_thing(string msg);

        public Form1()
        {
            InitializeComponent();

            // detect game folder location
            var appdata_path = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
            var path = appdata_path + "/FocasLens/幻想人形演舞-ユメノカケラ-/install.ini";
            if (!File.Exists(path))
            {
                path = appdata_path + "/FocasLens/幻想人形演舞/gn_enbu.ini";
                if (!File.Exists(path))
                    return;
            }

            try
            {
                var str = File.ReadAllText(path, Encoding.GetEncoding(932));
                if (String.IsNullOrEmpty(str))
                    return;
                var pos = str.IndexOf("InstallPath=");
                if (pos < 0)
                    return;
                pos += "InstallPath=".Length;
                var endpos = str.IndexOf("\r\n", pos);
                if (endpos < 0)
                    endpos = str.Length;
                textBox1.Text = str.Substring(pos, endpos - pos);
            }
            catch
            {
                textBox1.Text = "";
                return;
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (folderBrowserDialog1.ShowDialog() == DialogResult.OK)
                textBox1.Text = folderBrowserDialog1.SelectedPath;
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (openFileDialog1.ShowDialog() == DialogResult.OK)
                textBox2.Text = openFileDialog1.FileName;
        }

        public void append_msg(string msg)
        {
            textBox3.AppendText(msg);
        }

        private void append_err(string msg)
        {
            textBox3.SelectionStart = textBox3.TextLength;
            textBox3.SelectionLength = 0;
            textBox3.SelectionColor = Color.Red;
            textBox3.AppendText(msg);
            textBox3.SelectionColor = textBox3.ForeColor;
        }

        public void stdout_handler(object proc, DataReceivedEventArgs data)
        {
            if(!String.IsNullOrEmpty(data.Data))
            {
                if(textBox3.InvokeRequired)
                    textBox3.Invoke(new do_the_thing(append_msg), new object[] { (data.Data + "\r\n") });
                else
                    textBox3.AppendText(data.Data + "\r\n");
            }
        }

        public void stderr_handler(object proc, DataReceivedEventArgs data)
        {
            if(!String.IsNullOrEmpty(data.Data))
            {
                if(textBox3.InvokeRequired)
                    textBox3.Invoke(new do_the_thing(append_err), new object[] { (data.Data + "\r\n") });
                else
                    append_err(data.Data + "\r\n");
            }
        }

        public void onexit(Object source, EventArgs e)
        {
            proc_.Close();
            proc_ = null;
            button3.Enabled = true;
        }

        private void button3_Click(object sender, EventArgs e)
        {
            if(proc_ != null)
            {
                MessageBox.Show("Please wait for the current operation to complete.");
                return;
            }

            if(!File.Exists("diffgen.exe"))
            {
                MessageBox.Show("Could not find diffgen.exe, please make sure it is in the same folder as this program", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            textBox1.Text = textBox1.Text.TrimEnd("/\\".ToCharArray()); // Remove trailing slashes
            if(!Directory.Exists(textBox1.Text))
            {
                MessageBox.Show("Invalid game folder path", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            textBox2.Text = textBox2.Text.TrimEnd("/\\".ToCharArray()); // Remove trailing slashes
            if(!File.Exists(textBox2.Text))
            {
                MessageBox.Show("Invalid patch file path", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            textBox3.Clear();

            String args = "-i \"" + textBox1.Text + "\" -o \"" + textBox2.Text + "\" -p";
            textBox3.AppendText("diffgen.exe " + args + "\r\n");
            
            proc_ = new Process();
            proc_.StartInfo.FileName = "diffgen.exe";
            proc_.StartInfo.Arguments = args;
            proc_.StartInfo.UseShellExecute = false;
            proc_.StartInfo.RedirectStandardOutput = true;
            proc_.StartInfo.RedirectStandardError = true;
            proc_.StartInfo.CreateNoWindow = true;
            proc_.EnableRaisingEvents = true;
            proc_.OutputDataReceived += new DataReceivedEventHandler(stdout_handler);
            proc_.ErrorDataReceived += new DataReceivedEventHandler(stderr_handler);
            proc_.Exited += new EventHandler(onexit);
            proc_.SynchronizingObject = this;

            try
            {
                proc_.Start();
                proc_.BeginOutputReadLine();
                proc_.BeginErrorReadLine();
            }
            catch(Exception ex)
            {
                MessageBox.Show(ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                proc_.Close();
                proc_ = null;
            }

            button3.Enabled = false;
        }
    }
}

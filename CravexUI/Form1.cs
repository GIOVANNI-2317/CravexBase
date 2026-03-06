using System;
using System.IO;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace CravexUI
{
    public partial class Form1 : Form
    {
        [DllImport("Cravex.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        public static extern void attach(bool debug);

        [DllImport("Cravex.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        public static extern void execute(string input);

        [DllImport("Cravex.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool isAttached();

        [DllImport("Cravex.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
        public static extern void detach();

        public Form1()
        {
            InitializeComponent();
        }

        private void btnAttach_Click(object sender, EventArgs e)
        {
            attach(false);
        }

        private void btnExecute_Click(object sender, EventArgs e)
        {
            if (isAttached())
            {
                execute(richTextBox1.Text);
            }
            else
            {
                MessageBox.Show("Wait for attachment to complete, or click Attach first!", "Cravex Base");
            }
        }

        private void btnClear_Click(object sender, EventArgs e)
        {
            richTextBox1.Clear();
        }

        private void btnOpenFile_Click(object sender, EventArgs e)
        {
            using (OpenFileDialog ofd = new OpenFileDialog())
            {
                ofd.Filter = "Lua Scripts (*.lua;*.txt)|*.lua;*.txt|All files (*.*)|*.*";
                if (ofd.ShowDialog() == DialogResult.OK)
                {
                    richTextBox1.Text = File.ReadAllText(ofd.FileName);
                }
            }
        }

        private void btnSaveFile_Click(object sender, EventArgs e)
        {
            using (SaveFileDialog sfd = new SaveFileDialog())
            {
                sfd.Filter = "Lua Scripts (*.lua;*.txt)|*.lua;*.txt|All files (*.*)|*.*";
                if (sfd.ShowDialog() == DialogResult.OK)
                {
                    File.WriteAllText(sfd.FileName, richTextBox1.Text);
                }
            }
        }

        private void btnDetach_Click(object sender, EventArgs e)
        {
            if (isAttached())
            {
                detach();
            }
        }
    }
}

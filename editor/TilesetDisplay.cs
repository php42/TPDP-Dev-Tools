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
using editor.json;
using System.Runtime.Serialization.Json;
using System.Text.RegularExpressions;

namespace editor
{
    class TilesetDisplay : Control
    {
        private Bitmap bmp_;
        private int index_ = -1;

        public TilesetDisplay()
        {
            this.Cursor = Cursors.Hand;
        }

        public void SetBitmap(Bitmap bmp)
        {
            bmp_ = bmp;

            SetClientSizeCore(bmp_.Width, bmp_.Height);
            Invalidate();
            Update();
        }

        public int SelectedIndex
        {
            get { return index_; }
            set
            {
                index_ = value;
                Invalidate();
                Update();
            }
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            e.Graphics.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.NearestNeighbor;
            //e.Graphics.CompositingMode = System.Drawing.Drawing2D.CompositingMode.SourceCopy;
            e.Graphics.CompositingMode = System.Drawing.Drawing2D.CompositingMode.SourceOver;
            using(var b = new SolidBrush(this.BackColor))
            {
                e.Graphics.FillRectangle(b, e.ClipRectangle);
            }

            e.Graphics.DrawImage(bmp_, e.ClipRectangle, e.ClipRectangle, GraphicsUnit.Pixel);

            if(index_ >= 0)
            {
                using(var p = new Pen(Color.Red))
                {
                    p.Width = 3;
                    e.Graphics.DrawRectangle(p, new Rectangle((index_ % 8) * 16, (index_ / 8) * 16, 16, 16));
                }
            }
        }
    }
}
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
    class MapDisplay : Control
    {
        private Bitmap bmp_;

        public MapDisplay()
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

        protected override void OnPaint(PaintEventArgs e)
        {
            e.Graphics.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.NearestNeighbor;
            e.Graphics.CompositingMode = System.Drawing.Drawing2D.CompositingMode.SourceCopy;
            e.Graphics.DrawImage(bmp_, e.ClipRectangle, e.ClipRectangle, GraphicsUnit.Pixel);
        }

        public void UpdateIndex(Bitmap bmp, uint index)
        {
            var x = (((int)index * 16) % bmp_.Width) - (bmp.Width / 2);
            var y = ((((int)index * 16) / bmp_.Width) * 16) - (bmp.Height / 2);

            using(var g = Graphics.FromImage(bmp_))
            {
                g.CompositingMode = System.Drawing.Drawing2D.CompositingMode.SourceCopy;
                g.DrawImage(bmp, x, y);
            }

            Invalidate(new Rectangle(x, y, bmp.Width, bmp.Height));
            Update();
        }
    }
}
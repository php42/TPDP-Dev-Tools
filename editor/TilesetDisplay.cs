using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using System.IO;
using editor.bitmap;
using System.Runtime.Serialization.Json;
using System.Text.RegularExpressions;

namespace editor
{
    class TilesetDisplay : Control
    {
        private Bitmap bmp_;
        private int index_ = -1;

        public int Zoom { get; set; }

        public TilesetDisplay()
        {
            this.Cursor = Cursors.Hand;
            this.DoubleBuffered = true;
            Zoom = 1;
        }

        public void SetBitmap(Bitmap bmp)
        {
            if(Zoom == 1)
            {
                bmp_ = bmp;
            }
            else
            {
                var w = bmp.Width * Zoom;
                var h = bmp.Height * Zoom;
                bmp_ = BitmapUtil.Resize(bmp, w, h);
            }

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
            e.Graphics.InterpolationMode = InterpolationMode.NearestNeighbor;
            e.Graphics.CompositingMode = CompositingMode.SourceOver;
            e.Graphics.CompositingQuality = CompositingQuality.HighQuality;
            e.Graphics.PixelOffsetMode = PixelOffsetMode.HighQuality;
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
                    var tile_w = bmp_.Width / 8;
                    var tile_h = bmp_.Height / 32;
                    e.Graphics.DrawRectangle(p, new Rectangle((index_ % 8) * tile_w, (index_ / 8) * tile_h, tile_w, tile_h));
                }
            }
        }
    }
}
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
        private int zoom_ = 1;

        public int Zoom
        {
            get { return zoom_; }

            set
            {
                zoom_ = value;

                if(bmp_ != null)
                {
                    SetClientSizeCore(bmp_.Width * zoom_, bmp_.Height * zoom_);
                    Invalidate();
                    Update();
                }
            }
        }

        public TilesetDisplay()
        {
            this.Cursor = Cursors.Hand;
            this.DoubleBuffered = true;
            Zoom = 1;
        }

        public void SetBitmap(Bitmap bmp)
        {
            bmp_ = bmp;

            SetClientSizeCore(bmp_.Width * Zoom, bmp_.Height * Zoom);
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

            // align to tile boundary
            var tile_sz = 16 * Zoom;
            var dst = e.ClipRectangle;
            var dx = dst.X % tile_sz;
            var dy = dst.Y % tile_sz;
            dst.X -= dx;
            dst.Y -= dy;
            dst.Width += dx;
            dst.Height += dy;
            if((dst.Width % tile_sz) != 0)
                dst.Width += tile_sz - (dst.Width % tile_sz);
            if((dst.Height % tile_sz) != 0)
                dst.Height += tile_sz - (dst.Height % tile_sz);

            var src = dst;
            src.X /= Zoom;
            src.Y /= Zoom;
            src.Width /= Zoom;
            src.Height /= Zoom;

            e.Graphics.DrawImage(bmp_, dst, src, GraphicsUnit.Pixel);

            if(index_ >= 0)
            {
                using(var p = new Pen(Color.Red))
                {
                    p.Width = 3;
                    e.Graphics.DrawRectangle(p, new Rectangle((index_ % 8) * tile_sz, (index_ / 8) * tile_sz, tile_sz, tile_sz));
                }
            }
        }
    }
}
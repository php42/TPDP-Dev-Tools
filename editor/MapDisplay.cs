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

namespace editor
{
    class MapDisplay : Control
    {
        private Bitmap bmp_;
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

        public MapDisplay()
        {
            this.Cursor = Cursors.Hand;
            this.DoubleBuffered = true;
            Zoom = 1;
        }

        public void SetBitmap(Bitmap bmp)
        {
            if(bmp_ != null)
            {
                bmp_.Dispose();
                bmp_ = null;
            }

            bmp_ = bmp;

            SetClientSizeCore(bmp_.Width * Zoom, bmp_.Height * Zoom);
            Invalidate();
            Update();
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            e.Graphics.InterpolationMode = InterpolationMode.NearestNeighbor;
            e.Graphics.CompositingMode = CompositingMode.SourceCopy;
            e.Graphics.CompositingQuality = CompositingQuality.HighQuality;
            e.Graphics.PixelOffsetMode = PixelOffsetMode.HighQuality;

            // Fix alignment for autoscroll repaints
            // Cause winforms is the actual worst.
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
        }

        public void UpdateRegion(Bitmap bmp, int x, int y, bool repaint)
        {
            using(var g = Graphics.FromImage(bmp_))
            {
                g.InterpolationMode = InterpolationMode.NearestNeighbor;
                g.CompositingMode = CompositingMode.SourceCopy;
                g.CompositingQuality = CompositingQuality.HighQuality;
                g.PixelOffsetMode = PixelOffsetMode.HighQuality;

                g.DrawImage(bmp, x, y);
                Invalidate(new Rectangle(x * Zoom, y * Zoom, bmp.Width * Zoom, bmp.Height * Zoom));
            }

            if(repaint)
                Update();
        }
    }
}
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

            if(bmp_ != null)
                SetClientSizeCore(bmp_.Width * Zoom, bmp_.Height * Zoom);
            else
                SetClientSizeCore(0, 0);
            Invalidate();
            Update();
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            if(bmp_ == null)
            {
                base.OnPaint(e);
                return;
            }

            e.Graphics.InterpolationMode = InterpolationMode.NearestNeighbor;
            e.Graphics.CompositingMode = CompositingMode.SourceCopy;
            e.Graphics.CompositingQuality = CompositingQuality.HighQuality;
            e.Graphics.PixelOffsetMode = PixelOffsetMode.HighQuality;

            var dst = ClientRectangle;
            var src = dst;
            src.X /= Zoom;
            src.Y /= Zoom;
            src.Width /= Zoom;
            src.Height /= Zoom;

            e.Graphics.DrawImage(bmp_, dst, src, GraphicsUnit.Pixel);
        }

        public void UpdateRegion(Bitmap bmp, int x, int y, bool repaint)
        {
            if(bmp_ == null)
                return;

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
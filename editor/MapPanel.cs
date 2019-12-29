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
    class MapPanel : Panel
    {
        public MapPanel()
        {
            AutoScroll = true;
        }

        protected override Point ScrollToControl(Control activeControl)
        {
            return AutoScrollPosition;
        }

        protected override void OnMouseDown(MouseEventArgs e)
        {
            Focus();
            base.OnMouseDown(e);
        }

        protected override void OnScroll(ScrollEventArgs se)
        {
            if(GetScrollState(ScrollStateUserHasScrolled))
                Focus();
            base.OnScroll(se);
        }
    }
}
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace AfuueEditoor.View
{
    /// <summary>
    /// CurveEditView.xaml の相互作用ロジック
    /// </summary>
    public partial class CurveEditView : UserControl
    {
        public static readonly DependencyProperty CurvePointsProperty =
                DependencyProperty.Register(
                    "CurvePoints", // プロパティ名を指定
                    typeof(PointCollection), // プロパティの型を指定
                    typeof(CurveEditView), // プロパティを所有する型を指定
                    new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.AffectsRender));

        public PointCollection CurvePoints
        {
            get => (PointCollection)GetValue(CurvePointsProperty);
            set => SetValue(CurvePointsProperty, value);
        }

        public static readonly DependencyProperty ValueChangedProperty =
            DependencyProperty.Register(
                "ValueChanged",
                typeof(Action<int, double>),
                typeof(CurveEditView),
                new PropertyMetadata(null));
        public Action<int, double> ValueChanged
        {
            get => (Action<int, double>)GetValue(ValueChangedProperty);
            set => SetValue(ValueChangedProperty, value);
        }

        public CurveEditView()
        {
            InitializeComponent();
            Loaded += CurveEditView_Loaded;
        }

        private void CurveEditView_Loaded(object sender, RoutedEventArgs e)
        {
            Loaded -= CurveEditView_Loaded;
            MouseDown += Rect_MouseDown;
            MouseMove += Rect_MouseMove;
            MouseUp += Rect_MouseUp;
            zeroY = ActualHeight * 0.8;
            this.InvalidateVisual();
        }
        private Point? _mousePoint = null;
        private int _editingPointIndex = -1;
        private double zeroY = 0;

        private void Rect_MouseUp(object sender, MouseButtonEventArgs e)
        {
            if (_mousePoint != null)
            {
                this.ReleaseMouseCapture();
                _editingPointIndex = -1;
                this.InvalidateVisual();
            }
            _mousePoint = null;
        }

        private void Rect_MouseMove(object sender, MouseEventArgs e)
        {
            if (_mousePoint != null)
            {
                var mp = Mouse.GetPosition(this);
                mp.X /= ActualWidth;
                mp.Y /= zeroY;
                //Console.WriteLine("ValueChanged " + _editingPointIndex + "," + p.Y);
                ValueChanged(_editingPointIndex, mp.Y - ((Point)_mousePoint).Y);
                _mousePoint = mp;
                this.InvalidateVisual();
            }
        }

        private void Rect_MouseDown(object sender, MouseButtonEventArgs e)
        {
            var mp = Mouse.GetPosition(this);
            double dx = 5;
            double dy = 5;
            _editingPointIndex = -1;
            for (int i = 0; i < CurvePoints.Count; i++)
            {
                var p = CurvePoints[i];
                p.X *= ActualWidth;
                p.Y *= zeroY;
                if ((mp.X - dx < p.X) && (p.X < mp.X + dx) && (mp.Y - dy < p.Y) && (p.Y < mp.Y + dy))
                {
                    _mousePoint = new Point(mp.X / ActualWidth, mp.Y / zeroY);
                    _editingPointIndex = i;
                    this.CaptureMouse();
                    this.InvalidateVisual();
                    return;
                }
            }
        }

#if false
        private void UpdateRectPos()
        {
            if (CurvePoints == null)
            {
                return;
            }
            int i = 0;
            foreach (var obj in EditCanvas.Children)
            {
                if (i >= CurvePoints.Count)
                {
                    break;
                }
                var rect = obj as Rectangle;
                if (rect != null)
                {
                    var p = CurvePoints[i];
                    Canvas.SetLeft(rect, p.X * ActualWidth - rect.Width / 2);
                    Canvas.SetTop(rect, p.Y * zeroY - rect.Height / 2);
                }
                i++;
            }
            this.InvalidateVisual();
        }
#endif
        protected override void OnRender(DrawingContext drawingContext)
        {
            Pen gpen = new Pen(Brushes.Gray, 3);
            if (CurvePoints == null)
            {
                return;
            }
            Pen rpen = new Pen(Brushes.Red, 3);
            drawingContext.DrawLine(rpen, new Point(0, zeroY), new Point(ActualWidth, zeroY));

            for (int i = 0; i < CurvePoints.Count-1; i++)
            {
                var p0 = CurvePoints[i];
                p0.X *= ActualWidth;
                p0.Y *= zeroY;
                var p1 = CurvePoints[i + 1];
                p1.X *= ActualWidth;
                p1.Y *= zeroY;
                drawingContext.DrawLine(gpen, p0, p1);
            }
            for (int i = 0; i < CurvePoints.Count - 1; i++)
            {
                var p = CurvePoints[i];
                p.X *= ActualWidth;
                p.Y *= zeroY;
                if (i == _editingPointIndex)
                {
                    drawingContext.DrawRectangle(Brushes.Yellow, null, new Rect(p.X - 5, p.Y - 5, 10, 10));
                }
                else
                {
                    drawingContext.DrawRectangle(Brushes.Aqua, null, new Rect(p.X - 5, p.Y - 5, 10, 10));
                }
            }

            base.OnRender(drawingContext);
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
#if false
            string s = "double stepTable[] = {\n";
            foreach (var p in Points)
            {
                s += p.X + "," + p.Y + ",\n";
            }
            s += "};";
            System.IO.File.WriteAllText(@".\stepTable.txt", s);
#endif
        }
    }
}

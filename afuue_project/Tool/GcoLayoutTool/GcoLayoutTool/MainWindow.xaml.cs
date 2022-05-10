using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.IO;
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
using System.Windows.Media.Media3D;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;

/*
 * (0, 0) 
 *       ----------------> Y
 *       |           |
 *       |           |
 *       |     +     |  (up) Z
 *       |           |
 *       |           |
 *       -------------
 *       |
 *       v
 *       X
 *
 */

namespace GcoLayoutTool
{
    /// <summary>
    /// MainWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class MainWindow : Window
    {
        //--------------
        private DispatcherTimer timer;
        private double vAngle = 0.3;
        private double hAngle = 0.0;
        private double dis = 200;
        private double lineThickness = 0.2;
        private double stageSize = 100;
        private double objectHeight = 20;
        List<string> gcoHeader = new List<string>();
        List<string> gcoFooter = new List<string>();
        List<GcoObject> gcoObjects = new List<GcoObject>();

        private class GcoObject
        {
            public List<GcoCommand> commands = new List<GcoCommand>();
            public double eFirst = 0.0;
            public double eLast = 0.0;
            public Point3D pLast = new Point3D(0, 0, 0);
            public GeometryModel3D model;

            public void Add(GcoCommand command)
            {
                commands.Add(command);
                if (double.IsNaN(command.e) == false)
                {
                    eLast = command.e;
                }
                if (double.IsNaN(command.x) == false)
                {
                    pLast.X = command.x;
                }
                if (double.IsNaN(command.y) == false)
                {
                    pLast.Y = command.y;
                }
                if (double.IsNaN(command.z) == false)
                {
                    pLast.Z = command.z;
                }
            }
        }

        private class GcoCommand
        {
            /*
            G0 X57.883 Y57.6 F3120
            G1 X52.398 Y52.115 E9.51007 F1020
             */
            public string type;
            public double x = double.NaN;
            public double y = double.NaN;
            public double z = double.NaN;
            public double e = double.NaN;
            public double s = 0;
            public double f = 0;

            public GcoCommand(string command)
            {
                string[] sp = command.Split(' ');
                int i = 0;
                foreach (string sp1 in sp)
                {
                    if (i == 0)
                    {
                        type = sp1;
                        i++;
                        continue;
                    }
                    char c = sp1[0];
                    switch (c)
                    {
                        case 'X':
                            x = double.Parse(sp1.Substring(1));
                            break;
                        case 'Y':
                            y = double.Parse(sp1.Substring(1));
                            break;
                        case 'Z':
                            z = double.Parse(sp1.Substring(1));
                            break;
                        case 'E':
                            e = double.Parse(sp1.Substring(1)); // Extrude
                            break;
                        case 'F':
                            f = double.Parse(sp1.Substring(1)); // Speed(Fast)
                            break;
                        case 'S':
                            s = double.Parse(sp1.Substring(1)); // Temperature
                            break;
                        default:
                            System.Diagnostics.Debug.Assert(false);
                            break;
                    }
                    i++;
                }
            }
            public string GetCommandString()
            {
                string ret = type;
                if (double.IsNaN(x) == false)
                {
                    ret += " X" + String.Format("{0:F3}", x);
                }
                if (double.IsNaN(y) == false)
                {
                    ret += " Y" + String.Format("{0:F3}", y);
                }
                if (double.IsNaN(z) == false)
                {
                    ret += " Z" + String.Format("{0:F3}", z);
                }
                if (double.IsNaN(e) == false)
                {
                    ret += " E" + String.Format("{0:F5}", z);
                }
                if (f > 0)
                {
                    ret += " F" + f;
                }
                if (s > 0)
                {
                    ret += " S" + s;
                }
                return ret;
            }
            public void AddOffset(double ex, Point3D point)
            {
                if (double.IsNaN(e) == false)
                {
                    e += ex;
                }
                if (double.IsNaN(x) == false)
                {
                    x += point.X;
                }
                if (double.IsNaN(y) == false)
                {
                    y += point.Y;
                }
                if (double.IsNaN(z) == false)
                {
                    z += point.Z;
                }
            }
        }

        //--------------
        public MainWindow()
        {
            InitializeComponent();
        }

        //--------------
        private void AddLine3D(MeshGeometry3D geo, Point3D from, Point3D to, double thickness = 1)
        {
            Vector3D v = to - from;
            Vector3D vx = v;
            vx.Normalize();
            Vector3D vz;
            Vector3D vy;
            if (vx.Y >= 0.9)
            {
                vy = new Vector3D(1, 0, 0);
                vz = new Vector3D(0, 0, 1);
            }
            else
            {
                vz = Vector3D.CrossProduct(vx, new Vector3D(0, 1, 0));
                vy = Vector3D.CrossProduct(vz, vx);
            }
            vy.Normalize();
            vz.Normalize();
            vy *= thickness;
            vz *= thickness;
            geo.Positions.Add(from + vy / 2 - vz / 2);
            geo.Positions.Add(to + vy / 2 - vz / 2);
            geo.Positions.Add(from + vy / 2 + vz / 2);
            geo.Positions.Add(to + vy / 2 + vz / 2);
            geo.Positions.Add(from + vy / 2 + vz / 2);
            geo.Positions.Add(to + vy / 2 - vz / 2);
            geo.Normals.Add(-vy);
            geo.Normals.Add(-vy);
            geo.Normals.Add(-vy);
            geo.Normals.Add(-vy);
            geo.Normals.Add(-vy);
            geo.Normals.Add(-vy);
            geo.Positions.Add(from - vy / 2 - vz / 2);
            geo.Positions.Add(to - vy / 2 - vz / 2);
            geo.Positions.Add(from - vy / 2 + vz / 2);
            geo.Positions.Add(to - vy / 2 + vz / 2);
            geo.Positions.Add(from - vy / 2 + vz / 2);
            geo.Positions.Add(to - vy / 2 - vz / 2);
            geo.Normals.Add(vy);
            geo.Normals.Add(vy);
            geo.Normals.Add(vy);
            geo.Normals.Add(vy);
            geo.Normals.Add(vy);
            geo.Normals.Add(vy);
            geo.Positions.Add(from + vy / 2 + vz / 2);
            geo.Positions.Add(to + vy / 2 + vz / 2);
            geo.Positions.Add(from - vy / 2 + vz / 2);
            geo.Positions.Add(to - vy / 2 + vz / 2);
            geo.Positions.Add(from - vy / 2 + vz / 2);
            geo.Positions.Add(to + vy / 2 + vz / 2);
            geo.Normals.Add(vz);
            geo.Normals.Add(vz);
            geo.Normals.Add(vz);
            geo.Normals.Add(vz);
            geo.Normals.Add(vz);
            geo.Normals.Add(vz);
            geo.Positions.Add(from + vy / 2 - vz / 2);
            geo.Positions.Add(to + vy / 2 - vz / 2);
            geo.Positions.Add(from - vy / 2 - vz / 2);
            geo.Positions.Add(to - vy / 2 - vz / 2);
            geo.Positions.Add(from - vy / 2 - vz / 2);
            geo.Positions.Add(to + vy / 2 - vz / 2);
            geo.Normals.Add(-vz);
            geo.Normals.Add(-vz);
            geo.Normals.Add(-vz);
            geo.Normals.Add(-vz);
            geo.Normals.Add(-vz);
            geo.Normals.Add(-vz);
        }

        //--------------
        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            DiffuseMaterial mat = new DiffuseMaterial(Brushes.Aqua);
            MeshGeometry3D geo = new MeshGeometry3D();

            AddLine3D(geo, new Point3D(0.0, 0.0, 0.0), new Point3D(stageSize, 0.0, 0.0), lineThickness);
            AddLine3D(geo, new Point3D(stageSize, 0.0, 0.0), new Point3D(stageSize, 0.0, stageSize), lineThickness);
            AddLine3D(geo, new Point3D(stageSize, 0.0, stageSize), new Point3D(0.0, 0.0, stageSize), lineThickness);
            AddLine3D(geo, new Point3D(0.0, 0.0, stageSize), new Point3D(0.0, 0.0, 0.0), lineThickness);
            AddLine3D(geo, new Point3D(0.0, 0.0, 0.0), new Point3D(0.0, 100.0, 0.0), lineThickness);

            GeometryModel3D model = new GeometryModel3D(geo, mat);
            model.BackMaterial = mat;
            EditModel3DGroup.Children.Add(model);

            timer = new DispatcherTimer(new TimeSpan(0, 0, 0, 0, 100), DispatcherPriority.Normal, TimerTick, this.Dispatcher);
        }

        //--------------
        private void TimerTick(object sender, EventArgs e)
        {
            double x = dis * Math.Cos(hAngle);
            double z = dis * Math.Sin(hAngle);
            double y = dis * Math.Sin(vAngle);
            x = stageSize / 2.0 + x * Math.Cos(vAngle);
            z = stageSize / 2.0 + z * Math.Cos(vAngle);
            Camera.Position = new Point3D(x, y, z);
            Point3D lookAt = new Point3D(stageSize / 2.0, objectHeight / 2.0, stageSize / 2.0);
            Camera.LookDirection = lookAt - Camera.Position;
            Camera.LookDirection.Normalize();

            hAngle += 0.05;
            if (hAngle > Math.PI * 2)
            {
                hAngle -= Math.PI * 2;
            }
            InformationLabel.Content = "Angle " + String.Format("{0:F1}, {1:F1}", hAngle * 180 / Math.PI, vAngle * 180 / Math.PI);
        }

        //--------------
        private void AddButton_Click(object sender, RoutedEventArgs e)
        {
            timer.Stop();
            OpenFileDialog openFileDialog = new OpenFileDialog();
            openFileDialog.DefaultExt = ".gco";
            bool? ret = openFileDialog.ShowDialog();
            if ((ret != null) && (ret == true))
            {

                double extrudeLengthOffset = 0.0;
                Point3D pointOffset = new Point3D(0, 0, 0);
                GcoObject lastObj = null;
                //double eLast = 0.0;
                Point3D pLast = new Point3D(0, 0, 0);
                int gCount = 0;
                if (gcoObjects.Count > 0)
                {
                    lastObj = gcoObjects.Last();
                    extrudeLengthOffset = lastObj.eLast;
                }
                switch (gcoObjects.Count)
                {
                    case 0:
                        pointOffset.X -= 20;
                        pointOffset.Y -= 20;
                        break;
                    case 1:
                        pointOffset.X -= 20;
                        pointOffset.Y -= 0;
                        break;
                    case 2:
                        pointOffset.X -= 20;
                        pointOffset.Y += 20;
                        break;
                    case 3:
                        pointOffset.X -= 0;
                        pointOffset.Y -= 20;
                        break;
                    case 4:
                        pointOffset.X -= 0;
                        pointOffset.Y -= 0;
                        break;
                    case 5:
                        pointOffset.X -= 0;
                        pointOffset.Y += 20;
                        break;
                    case 6:
                        pointOffset.X += 20;
                        pointOffset.Y -= 20;
                        break;
                    case 7:
                        pointOffset.X += 20;
                        pointOffset.Y -= 0;
                        break;
                    case 8:
                        pointOffset.X += 20;
                        pointOffset.Y += 20;
                        break;
                }
                using (FileStream fs = new FileStream(openFileDialog.FileName, FileMode.Open))
                {
                    using (StreamReader sr = new StreamReader(fs))
                    {
                        bool waitForFirstM106 = true;
                        bool waitForLastM107 = true;
                        GcoObject gcoObject = new GcoObject();
                        while (sr.EndOfStream == false)
                        {
                            string line = sr.ReadLine();
                            if (waitForFirstM106)
                            {
                                if (line.StartsWith("M106"))
                                {
                                    GcoCommand command = new GcoCommand(line);
                                    command.AddOffset(extrudeLengthOffset, pointOffset);
                                    gcoObject.Add(command);
                                    waitForFirstM106 = false;
                                }
                                else
                                {
                                    if (lastObj == null)
                                    {
                                        gcoHeader.Add(line);
                                    }
                                }
                            }
                            else if (waitForLastM107)
                            {
                                if (line.StartsWith("M107"))
                                {
                                    if (lastObj == null)
                                    {
                                        gcoFooter.Add(line);
                                    }
                                    waitForLastM107 = false;
                                }
                                else
                                {
                                    GcoCommand command = new GcoCommand(line);
                                    command.AddOffset(extrudeLengthOffset, pointOffset);
                                    gcoObject.Add(command);

                                    if ((lastObj != null) && (command.type.StartsWith("G")))
                                    {
                                        if (gCount == 0)
                                        {
                                            GcoCommand moveHorizontalCommand = new GcoCommand(line);
                                            moveHorizontalCommand.type = "G0";
                                            moveHorizontalCommand.e = lastObj.eLast - 6;  // 6mm 引き戻し
                                            moveHorizontalCommand.z = lastObj.pLast.Z;    // 高さそのまま
                                            lastObj.commands.Add(moveHorizontalCommand);

                                            GcoCommand moveVerticalCommand = new GcoCommand(line);
                                            moveVerticalCommand.type = "G0";
                                            moveVerticalCommand.e = lastObj.eLast; // 引き戻しやめ、高さは指示の場所に
                                            lastObj.commands.Add(moveVerticalCommand);
                                        }
                                        gCount++;
                                    }
                                }
                            }
                            else
                            {
                                if (lastObj == null)
                                {
                                    gcoFooter.Add(line);
                                }
                            }
                        }
                        gcoObject.eFirst = gcoObject.commands.First().e;
                        gcoObject.eLast = gcoObject.commands.Last().e;
                        gcoObjects.Add(gcoObject);

                        DiffuseMaterial mat = new DiffuseMaterial(Brushes.Red);
                        MeshGeometry3D geo = new MeshGeometry3D();

                        Point3D p = new Point3D(0, 0, 0);
                        foreach (var command in gcoObject.commands)
                        {
                            if (command.type == "G0")
                            {
                                if (double.IsNaN(command.x) == false) p.X = command.x;
                                if (double.IsNaN(command.y) == false) p.Z = command.y;
                                if (double.IsNaN(command.z) == false) p.Y = command.z;
                            }
                            if (command.type == "G1")
                            {
                                Point3D q = new Point3D(p.X, p.Y, p.Z);
                                if (double.IsNaN(command.x) == false) q.X = command.x;
                                if (double.IsNaN(command.y) == false) q.Z = command.y;
                                if (double.IsNaN(command.z) == false) q.Y = command.z;
                                AddLine3D(geo, p, q, lineThickness);
                                p = q;
                            }
                        }

                        gcoObject.model = new GeometryModel3D(geo, mat);
                        gcoObject.model.BackMaterial = mat;
                        EditModel3DGroup.Children.Add(gcoObject.model);

                    }
                }
            }
            timer.Start();
        }

        //--------------
        private void SaveButton_Click(object sender, RoutedEventArgs e)
        {
            SaveFileDialog saveFileDialog = new SaveFileDialog();
            saveFileDialog.DefaultExt = ".gco";
            bool? ret = saveFileDialog.ShowDialog();
            if ((ret != null) && (ret == true))
            {
                using (FileStream fs = new FileStream(saveFileDialog.FileName, FileMode.Create))
                {
                    using (StreamWriter sw = new StreamWriter(fs))
                    {
                        foreach (var s in gcoHeader)
                        {
                            sw.WriteLine(s);
                        }

                        foreach (var gcoObj in gcoObjects)
                        {
                            foreach (var commands in gcoObj.commands)
                            {
                                var s = commands.GetCommandString();
                                sw.WriteLine(s);
                            }
                        }

                        foreach (var s in gcoFooter)
                        {
                            sw.WriteLine(s);
                        }
                    }
                }
            }
        }
    }
}

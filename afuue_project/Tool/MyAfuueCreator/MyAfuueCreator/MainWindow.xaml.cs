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
using System.IO.Ports;
using System.Windows.Threading;

namespace MyAfuueCreator
{
    /// <summary>
    /// MainWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class MainWindow : Window
    {
        private SerialPort serialPort = new SerialPort();
        private const int BUFFERSIZE = 4096;
        private byte[] receiveBuffer = new byte[BUFFERSIZE];
        private int receivePos = 0;
        private bool waitForCommand = true;
        private bool stopSendChangeEvent = false;
        private string reqSendChangeEvent = "";
        private bool stopViewChangedEvent = false;
        private DispatcherTimer dispatcherTimer = null;

        private List<double> wavePoints = new List<double>();
        private List<double> shiftTable = new List<double>();

        //------------------
        public MainWindow()
        {
            InitializeComponent();

            serialPort.BaudRate = 115200;
            serialPort.Parity = Parity.None;
            serialPort.DataBits = 8;
            serialPort.StopBits = StopBits.One;
            serialPort.Handshake = Handshake.None;
            serialPort.DataReceived += SerialPort_DataReceived;
            serialPort.ErrorReceived += (sender, e) => { MessageBox.Show("serialError" + e.EventType.ToString()); };

            dispatcherTimer = new DispatcherTimer(DispatcherPriority.ContextIdle, this.Dispatcher);
            dispatcherTimer.Interval = new TimeSpan(0, 0, 0, 0, 200);
            dispatcherTimer.Tick += DispatcherTimer_Tick;
            dispatcherTimer.Start();
        }

        //------------------
        private void DispatcherTimer_Tick(object sender, EventArgs e)
        {
            if (WaveCanvas.Children.Count > 0)
            {
                double w = WaveCanvas.ActualWidth;
                double h = WaveCanvas.ActualHeight;
                int i = 0;
                foreach (var item in WaveCanvas.Children)
                {
                    if (item.GetType() == typeof(Line))
                    {
                        if (i > 1)
                        {
                            int j = i - 2;
                            Line line = item as Line;
                            double x = ((j % 256) * 2);
                            double y1 = h / 2 - (h / 2) * wavePoints[(j % 256) + (j / 256) * 256];
                            double y2 = h / 2 - (h / 2) * wavePoints[((j + 1) % 256) + (j / 256) * 256];
                            line.X1 = x;
                            line.X2 = x + 2;
                            line.Y1 = y1;
                            line.Y2 = y2;
                        }

                        i++;
                    }
                }
            }
            if (ShiftCanvas.Children.Count > 0)
            {
                double w = ShiftCanvas.ActualWidth;
                double h = ShiftCanvas.ActualHeight;
                int i = 0;
                foreach (var item in ShiftCanvas.Children)
                {
                    if (item.GetType() == typeof(Line))
                    {
                        if (i > 0)
                        {
                            int j = i - 1;
                            Line line = item as Line;
                            double x = ((j % 32) * (512.0/31.0));
                            double y1 = h - h * shiftTable[j];
                            double y2 = h - h * shiftTable[j + 1];
                            line.X1 = x;
                            line.X2 = x + (512.0/31.0);
                            line.Y1 = y1;
                            line.Y2 = y2;
                        }

                        i++;
                    }
                }
            }
        }

        //------------------
        private void WriteLog(string s)
        {
            this.Dispatcher.Invoke((Action)(() =>
            {
                LogTextBox.AppendText(s + "\n");
                LogTextBox.ScrollToEnd();
            }));
        }

        //------------------
        private void Connect()
        {
            if (serialPort.IsOpen)
            {
                serialPort.Close();
                System.Threading.Thread.Sleep(1000);
            }
            if (SerialPortComboBox.Text.StartsWith("COM"))
            {
                try
                {
                    serialPort.PortName = SerialPortComboBox.Text;
                    serialPort.Open();
                    System.Threading.Thread.Sleep(1000);
                    GetAllToneData();
                }
                catch (Exception)
                {
                }
            }
        }

        //------------------
        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            {
                double w = WaveCanvas.ActualWidth;
                double h = WaveCanvas.ActualHeight;
                Rectangle rect = new Rectangle();
                rect.Width = w;
                rect.Height = h;
                rect.Fill = new SolidColorBrush(Color.FromArgb(255, 30, 50, 30));
                WaveCanvas.Children.Add(rect);

                {
                    Line line = new Line();
                    line.X1 = 0;
                    line.X2 = w;
                    line.Y1 = line.Y2 = h / 2;
                    line.Stroke = Brushes.Green;
                    WaveCanvas.Children.Add(line);
                }
                {
                    Line line = new Line();
                    line.X1 = line.X2 = w / 2;
                    line.Y1 = 0;
                    line.Y2 = h;
                    line.Stroke = Brushes.Green;
                    WaveCanvas.Children.Add(line);
                }

                for (int i = 0; i < 256; i++)
                {
                    wavePoints.Add(0);

                    Line box = new Line();
                    box.Stroke = new SolidColorBrush(Color.FromArgb(255, 255, 192, 32));
                    WaveCanvas.Children.Add(box);
                }
                for (int i = 0; i < 256; i++)
                {
                    wavePoints.Add(0);

                    Line waveLine = new Line();
                    waveLine.Stroke = new SolidColorBrush(Color.FromArgb(255, 32, 192, 255));
                    WaveCanvas.Children.Add(waveLine);
                }
            }
            {
                double w = ShiftCanvas.ActualWidth;
                double h = ShiftCanvas.ActualHeight;
                Rectangle rect = new Rectangle();
                rect.Width = w;
                rect.Height = h;
                rect.Fill = new SolidColorBrush(Color.FromArgb(255, 30, 30, 50));
                ShiftCanvas.Children.Add(rect);

                Line line = new Line();
                line.X1 = 0;
                line.X2 = w;
                line.Y1 = line.Y2 = h / 2;
                line.Stroke = Brushes.Blue;
                ShiftCanvas.Children.Add(line);

                for (int i = 0; i < 32; i++)
                {
                    shiftTable.Add(0);
                    if (i == 31) break;

                    Line shiftLine = new Line();
                    shiftLine.Stroke = new SolidColorBrush(Color.FromArgb(255, 130, 192, 255));
                    ShiftCanvas.Children.Add(shiftLine);
                }
            }
            string[] ports = SerialPort.GetPortNames();
            foreach (string port in ports)
            {
                SerialPortComboBox.Items.Add(port);
            }
            if (SerialPortComboBox.Items.Count > 0)
            {
                SerialPortComboBox.SelectedIndex = 0;
                Connect();
            }
        }

        //------------------
        private void SerialPortComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            Connect();
        }

        //------------------
        // -64 to 63
        private int ChangeSigned(int data)
        {
            if (data < 64)
            {
                return data;
            }
            else
            {
                return data - 128;
            }
        }

        //------------------
        private int ChangeUnsigned(int data)
        {
            if (data >= 0)
            {
                return data;
            }
            else
            {
                return 128 + data;
            }
        }

        //------------------
        private void OnReceiveCommand()
        {
            /*
             * 0xA1 SET TONE NUMBER
             * 0xA2 SET TRANSPOSE
             * 0xA3 SET FINE TUNE
             * 0xA4 SET REVERB LEVEL
             * 0xA5 SET PORTAMENT LEVEL
             * 0xA6 SET KEY SENSITIVITY
             * 0xA7 SET BREATH SENSITIVITY
             * 0xAA SET WAVE A DATA
             * 0xAB SET WAVE B DATA
             * 
             * 0xB1 GET TONE NUMBER
             * 0xB2 GET TRANSPOSE
             * 0xB3 GET FINE TUNE
             * 0xB4 GET REVERB LEVEL
             * 0xB5 GET PORTAMENT LEVEL
             * 0xB6 GET KEY SENSITIVITY
             * 0xB7 GET BREATH SENSITIVITY
             * 0xBA GET WAVE A DATA
             * 0xBB GET WAVE B DATA
             * 
             * 0xC1 RESPONSE TONE NUMBER
             * 0xC2 RESPONSE TRANSPOSE
             * 0xC3 RESPONSE FINE TUNE
             * 0xC4 RESPONSE REVERB LEVEL
             * 0xC5 RESPONSE PORTAMENT LEVEL
             * 0xC6 RESPONSE KEY SENSITIVITY
             * 0xC7 RESPONSE BREATH SENSITIVITY
             * 0xCA RESPONSE WAVE A DATA
             * 0xCB RESPONSE WAVE B DATA
             * 
             * 0xE1 STORE CONFIGS
             * 
             * 0xEE SOFTWARE RESET
             * 
             * 0xFE COMMAND ACK
             * 0xFF END MESSAGE
             */
            int command = receiveBuffer[0];
            WriteLog("command 0x" + String.Format("{0:X2}", command));
            stopViewChangedEvent = true;
            switch (command)
            {
                case 0xA1: // SET TONE NUMBER
                case 0xC1: // RESPONSE TONE NUMBER
                    {
                        int data = receiveBuffer[1];
                        this.Dispatcher.Invoke((Action)(() =>
                        {
                            ToneComboBox.SelectedIndex = data;
                        }));
                    }
                    break;
                case 0xA2: // SET TRANSPOSE
                case 0xC2: // RESPONSE TRANSPOSE
                    {
                        int data = ChangeSigned(receiveBuffer[1]);
                        this.Dispatcher.Invoke((Action)(() =>
                        {
                            TransposeComboBox.SelectedIndex = data + 12;
                        }));
                    }
                    break;
                case 0xA3: // SET FINE TUNE
                case 0xC3: // RESPONSE FINE TUNE
                    {
                        int data = ChangeSigned(receiveBuffer[1]);
                        this.Dispatcher.Invoke((Action)(() =>
                        {
                            string t = (data + 440).ToString();
                            int i = 0;
                            foreach (var item in TuneComboBox.Items)
                            {
                                Label label = item as Label;
                                if ((label.Content as string).StartsWith(t))
                                {
                                    TuneComboBox.SelectedIndex = i;
                                    break;
                                }
                                i++;
                            }
                        }));
                    }
                    break;
                case 0xA4: // SET REVERB LEVEL
                case 0xC4: // RESPONSE REVERB LEVEL
                    {
                        int data = ChangeSigned(receiveBuffer[1]);
                        this.Dispatcher.Invoke((Action)(() =>
                        {
                            ReverbLevelSlider.Value = data;
                        }));
                    }
                    break;
                case 0xA5: // SET PORTAMENT LEVEL
                case 0xC5: // RESPONSE PORTAMENT LEVEL
                    {
                        int data = ChangeSigned(receiveBuffer[1]);
                        this.Dispatcher.Invoke((Action)(() =>
                        {
                            PortamentoLevelSlider.Value = data;
                        }));
                    }
                    break;
                case 0xA6: // SET KEY SENSITIVITY
                case 0xC6: // RESPONSE KEY SENSITIVITY
                    {
                        int data = ChangeSigned(receiveBuffer[1]);
                        this.Dispatcher.Invoke((Action)(() =>
                        {
                            KeySensitivityLevelSlider.Value = data;
                        }));
                    }
                    break;
                case 0xA7: // SET BREATH SENSITIVITY
                case 0xC7: // RESPONSE BREATH SENSITIVITY
                    {
                        int data = ChangeSigned(receiveBuffer[1]);
                        this.Dispatcher.Invoke((Action)(() =>
                        {
                            BreathSensitivityLevelSlider.Value = data;
                        }));
                    }
                    break;
                case 0xAA: // SET WAVE A DATA
                case 0xCA: // RESPONSE WAVE A DATA
                case 0xAB: // SET WAVE B DATA
                case 0xCB: // RESPONSE WAVE B DATA
                    if (receivePos >= 256) {
                        int offset = 0;
                        if ((command & 0x0F) == 0x0B)
                        {
                            offset = 256;
                        }
                        for (int i = 0; i < 256; i++)
                        {
                            int d0 = receiveBuffer[1 + i * 2];
                            int d1 = receiveBuffer[1 + i * 2 + 1];
                            int data = (d0 << 2) | (d1 << 9);
                            if (data >= 0x8000)
                            {
                                data = data - 0x10000;
                            }
                            wavePoints[i + offset] = data / 32767.0;
                        }
                    }
                    break;
                case 0xAC: // SET SHIFT TABLE
                case 0xCC: // RESPONSE SHIFT TABLE
                    if (receivePos >= 32)
                    {
                        for (int i = 0; i < 32; i++)
                        {
                            shiftTable[i] = receiveBuffer[1 + i] / 127.0;
                        }
                    }
                    break;
                case 0xFE: // COMMAND ACK
                    WriteLog("(ACK)");
                    break;
            }
            stopViewChangedEvent = false;

            waitForCommand = true;
            receivePos = 0;
        }

        //------------------
        private void SerialPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            if (serialPort.IsOpen == false) return;

            byte[] buff = new byte[2048];
            int readSize = serialPort.Read(buff, 0, 2048);
            for (int i = 0; i < readSize; i++)
            {
                int d = buff[i];
                if (d == 0xFF)
                {
                    WriteLog("ready");
                }
                if (waitForCommand)
                {
                    if ((d >= 0x80) && (d != 0xFF))
                    {
                        receivePos = 0;
                        receiveBuffer[receivePos++] = (byte)d;
                        waitForCommand = false;
                    }
                }
                else
                {
                    if (d < 0x80)
                    {
                        if (receivePos < BUFFERSIZE)
                        {
                            receiveBuffer[receivePos++] = (byte)d;
                        }
                    }
                    else
                    {
                        OnReceiveCommand();
                        if (d < 0xFF)
                        {
                            receivePos = 0;
                            if (receivePos < BUFFERSIZE)
                            {
                                receiveBuffer[receivePos++] = (byte)d;
                            }
                            waitForCommand = false;
                        }
                    }
                }
            }
        }

        //------------------
        private void GetToneNumber()
        {
            if (serialPort.IsOpen == false) return;

            byte[] buff = new byte[256];
            buff[0] = 0xB1; // GET TONE NUMBER
            buff[1] = 0xFF;
            serialPort.Write(buff, 0, 2);

            WriteLog("send get toneNo");
        }

        //------------------
        private void SendToneNumber()
        {
            if (serialPort.IsOpen == false) return;

            byte[] buff = new byte[256];
            buff[0] = 0xA1; // SET TONE NUMBER
            buff[1] = (byte)(ToneComboBox.SelectedIndex);
            buff[2] = 0xFF;
            serialPort.Write(buff, 0, 3);

            WriteLog("send set toneNo");
        }

        //------------------
        private void SendAllToneData()
        {
            if (serialPort.IsOpen == false) return;

            byte[] buff = new byte[256];
            buff[0] = 0xA1; // SET TONE NUMBER
            buff[1] = (byte)(ToneComboBox.SelectedIndex);
            buff[2] = 0xA2; // SET TRANSPOSE
            buff[3] = (byte)ChangeUnsigned(TransposeComboBox.SelectedIndex - 12);
            buff[4] = 0xA3; // SET FINE TUNE
            Label label = TuneComboBox.SelectedItem as Label;
            string[] tunestr = (label.Content as string).Split(' ');
            int tune = int.Parse(tunestr[0]) - 440;
            buff[5] = (byte)ChangeUnsigned(tune);
            buff[6] = 0xA4; // SET REVERB LEVEL
            buff[7] = (byte)ReverbLevelSlider.Value;
            buff[8] = 0xA5; // SET PORTAMENT LEVEL
            buff[9] = (byte)PortamentoLevelSlider.Value;
            buff[10] = 0xA6; // SET KEY SENSITIVITY
            buff[11] = (byte)KeySensitivityLevelSlider.Value;
            buff[12] = 0xA7; // SET BREATH SENSITIVIY
            buff[13] = (byte)BreathSensitivityLevelSlider.Value;
            buff[14] = 0xFF;
            serialPort.Write(buff, 0, 15);

            WriteLog("send set all");
        }
        //------------------
        private void GetAllToneData()
        {
            if (serialPort.IsOpen == false) return;

            byte[] buff = new byte[256];
            buff[0] = 0xB1; // GET TONE NUMBER
            buff[1] = 0xB2; // GET TRANSPOSE
            buff[2] = 0xB3; // GET FINE TUNE
            buff[3] = 0xB4; // GET REVERB LEVEL
            buff[4] = 0xB5; // GET PORTAMENT LEVEL
            buff[5] = 0xB6; // GET KEY SENSITIVITY
            buff[6] = 0xB7; // GET BREATH SENSITIVIY
            buff[7] = 0xBA; // GET WAVE A DATA
            buff[8] = 0xBB; // GET WAVE B DATA
            buff[9] = 0xBC; // GET SHIFT TABLE
            buff[10] = 0xFF;
            serialPort.Write(buff, 0, 11);

            WriteLog("send get all");
        }

        //------------------
        private void SendWaveData()
        {
            if (serialPort.IsOpen == false) return;

            byte[] buff = new byte[600];
            buff[0] = 0xAA; // SET WAVE A DATA
            int offset = 0;
            if ((bool)RadioButtonWaveB.IsChecked)
            {
                offset = 256;
            }
            for (int i = 0; i < 256; i++)
            {
                int data = (int)(32767 * wavePoints[i + offset]);
                if (data < -32767) data = -32767;
                if (data > 32767) data = 32767;
                if (data < 0)
                {
                    data = 0x10000 + data;
                }
                int d0 = (data >> 2) & 0x7F;
                int d1 = (data >> 9) & 0x7F;
                buff[1 + i * 2] = (byte)(d0);
                buff[1 + i * 2 + 1] = (byte)(d1);
            }
            buff[513] = 0xFF;
            serialPort.Write(buff, 0, 514);

            WriteLog("send set wave data");
        }

        //------------------
        private void SendShiftTable()
        {
            if (serialPort.IsOpen == false) return;

            byte[] buff = new byte[600];
            buff[0] = 0xAC; // SET SHIFT TABLE
            for (int i = 0; i < 32; i++)
            {
                int data = (int)(127 * shiftTable[i]);
                if (data > 127) data = 127;
                if (data < 0)
                {
                    data = 0;
                }
                buff[1 + i] = (byte)(data);
            }
            buff[33] = 0xFF;
            serialPort.Write(buff, 0, 34);

            WriteLog("send shift table");
        }

        //------------------
        private void WriteButton_Click(object sender, RoutedEventArgs e)
        {
            byte[] buff = new byte[4];
            buff[0] = 0xE1;
            buff[1] = 0x7F;
            buff[2] = 0xFF;
            serialPort.Write(buff, 0, 3);
        }

        //------------------
        private void ToneComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (stopViewChangedEvent) return;
            SendToneNumber();
            System.Threading.Thread.Sleep(500);
            GetAllToneData();
        }

        //------------------
        private void ToneValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (stopViewChangedEvent) return;
            if (sender == ReverbLevelSlider)
            {
                reqSendChangeEvent = "Reverb";
            }
            else if (sender == PortamentoLevelSlider)
            {
                reqSendChangeEvent = "Portamento";
            }
            else if (sender == KeySensitivityLevelSlider)
            {
                reqSendChangeEvent = "KeySense";
            }
            else if (sender == BreathSensitivityLevelSlider)
            {
                reqSendChangeEvent = "BreathSense";
            }
        }

        //------------------
        private void ToneValueChanged2(object sender, SelectionChangedEventArgs e)
        {
            if (stopViewChangedEvent) return;
            SendAllToneData();
        }

        private Point? waveCanvasMousePoint = null;
        //------------------
        private void WaveCanvas_MouseDown(object sender, MouseButtonEventArgs e)
        {
            waveCanvasMousePoint = e.GetPosition(WaveCanvas);
            WaveCanvas.CaptureMouse();
        }

        //------------------
        private void WaveCanvas_MouseMove(object sender, MouseEventArgs e)
        {
            if (waveCanvasMousePoint == null) return;

            double w = WaveCanvas.ActualWidth;
            double h = WaveCanvas.ActualHeight;

            Point p = e.GetPosition(WaveCanvas);
            double mx = ((Point)waveCanvasMousePoint).X;
            double my = ((Point)waveCanvasMousePoint).Y;
            double dx = p.X - mx;
            double dy = p.Y - my;

            int offset = 0;
            if ((bool)RadioButtonWaveB.IsChecked)
            {
                offset = 256;
            }
            for (int i = 0; i < 256; i++)
            {
                double x = i * w / 256.0;
                if ( ((mx <= x) && (x <= p.X)) || ((p.X <= x) && (x <= mx)) )
                {
                    double v = -((my + (dy * i) / 256) - h / 2) / 127.0;
                    if (v > 1.0) v = 1.0;
                    if (v < -1.0) v = -1.0;
                    wavePoints[i + offset] = v;
                }
            }
            waveCanvasMousePoint = p;
        }

        //------------------
        private void WaveCanvas_MouseUp(object sender, MouseButtonEventArgs e)
        {
            waveCanvasMousePoint = null;
            WaveCanvas.ReleaseMouseCapture();
            SendWaveData();
        }

        private Point? shiftCanvasMousePoint = null;
        //------------------
        private void ShiftCanvas_MouseDown(object sender, MouseButtonEventArgs e)
        {
            shiftCanvasMousePoint = e.GetPosition(ShiftCanvas);
            ShiftCanvas.CaptureMouse();
        }

        //------------------
        private void ShiftCanvas_MouseMove(object sender, MouseEventArgs e)
        {
            if (shiftCanvasMousePoint == null) return;

            double w = ShiftCanvas.ActualWidth;
            double h = ShiftCanvas.ActualHeight;

            Point p = e.GetPosition(ShiftCanvas);
            double mx = ((Point)shiftCanvasMousePoint).X;
            double my = ((Point)shiftCanvasMousePoint).Y;
            double dx = p.X - mx;
            double dy = p.Y - my;

            for (int i = 0; i < 32; i++)
            {
                double x = i * w / 32.0;
                if (((mx <= x) && (x <= p.X)) || ((p.X <= x) && (x <= mx)))
                {
                    double v = (256.0 - p.Y) / 256.0;
                    if (v < 0.0) v = 0.0;
                    if (v > 1.0) v = 1.0;
                    shiftTable[i] = v;
                }
            }
            shiftCanvasMousePoint = p;
        }

        //------------------
        private void ShiftCanvas_MouseUp(object sender, MouseButtonEventArgs e)
        {
            shiftCanvasMousePoint = null;
            ShiftCanvas.ReleaseMouseCapture();
            SendShiftTable();
        }

        //------------------
        private void RebootButton_Click(object sender, RoutedEventArgs e)
        {
            byte[] buff = new byte[4];
            buff[0] = 0xEE;
            buff[1] = 0x7F;
            buff[2] = 0xFF;
            serialPort.Write(buff, 0, 3);
            //System.Threading.Thread.Sleep(3000);
            //Connect();
        }

        //------------------
        private void Slider_MouseDown(object sender, MouseButtonEventArgs e)
        {
            stopSendChangeEvent = true;
        }

        //------------------
        private void Slider_MouseUp(object sender, MouseButtonEventArgs e)
        {
            stopSendChangeEvent = false;
            if (serialPort.IsOpen == false) return;

            if (reqSendChangeEvent == "Reverb")
            {
                byte[] buff = new byte[256];
                buff[0] = 0xA4; // SET REVERB LEVEL
                buff[1] = (byte)ReverbLevelSlider.Value;
                buff[2] = 0xFF;
                serialPort.Write(buff, 0, 3);
            }
            if (reqSendChangeEvent == "Portamento")
            {
                byte[] buff = new byte[256];
                buff[0] = 0xA5; // SET PORTAMENT LEVEL
                buff[1] = (byte)PortamentoLevelSlider.Value;
                buff[2] = 0xFF;
                serialPort.Write(buff, 0, 3);
            }
            if (reqSendChangeEvent == "KeySense")
            {
                byte[] buff = new byte[256];
                buff[0] = 0xA6; // SET KEY SENSITIVITY
                buff[1] = (byte)KeySensitivityLevelSlider.Value;
                buff[2] = 0xFF;
                serialPort.Write(buff, 0, 3);
            }
            if (reqSendChangeEvent == "BreathSense")
            {
                byte[] buff = new byte[256];
                buff[0] = 0xA7; // SET BREATH SENSITIVIY
                buff[1] = (byte)BreathSensitivityLevelSlider.Value;
                buff[2] = 0xFF;
                serialPort.Write(buff, 0, 3);
            }
            if (reqSendChangeEvent != "")
            {
                WriteLog("send " + reqSendChangeEvent);
            }
            reqSendChangeEvent = "";
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            // Are you sure?

            if (serialPort.IsOpen == false) return;

            byte[] buff = new byte[256];
            buff[0] = 0xEE; // RESET
            buff[1] = 0x7A;
            buff[2] = 0x3C;
            buff[3] = 0xFF;
            serialPort.Write(buff, 0, 4);
            WriteLog("factory reset");

            System.Threading.Thread.Sleep(5000);

            GetAllToneData();
        }
    }
}

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
        private const Byte commVer1 = 0x15;
        private const Byte commVer2 = 0x02;

        public class ToneSetting
        {
            public bool RequestSynchronize = true;
            
            public SByte Transpose = 0;
            public SByte FineTune = 0;
            public Byte Reverb = 7;
            public Byte Portamento = 0;
            public Byte KeySense = 0;
            public Byte BreathSense = 0;
            public Int16[] WaveA = new Int16[256];
            public Int16[] WaveB = new Int16[256];
            public Byte[] ShiftTable = new Byte[32];
            public Byte[] NoiseTable = new Byte[32];
            public Byte[] PitchTable = new Byte[32];
        }

        public class GlobalSettings
        {
            public Byte ToneNo = 0;
            public Byte Metronome_m = 1;
            public Byte Metronome_v = 0;
            public int Metronome_t = 90;
            public Byte MidiControlType = 0;
            public Byte MidiPgNo = 51;
        };

        public class SaveData
        {
            public GlobalSettings GlobalSetting = new GlobalSettings();
            public ToneSetting[] ToneSettings { get; set; } = new ToneSetting[5];
        }

        public SaveData saveData = new SaveData();

        public class SerialCommand
        {
            public bool sended = false;
            public Action ackAction = null;
            public bool noWaitResponse = false;
            public long sendTime = 0;

            public byte[] sendBuffer = null;
            public void SetBufferSize(int size)
            {
                sendBuffer = new byte[size];
                for (int i =  0; i < size; i++)
                {
                    sendBuffer[i] = 0;
                }
            }
        }
        public List<SerialCommand> commandQueue = new List<SerialCommand>();

        private SerialPort serialPort = new SerialPort();
        private const int BUFFERSIZE = 4096;
        private byte[] receiveBuffer = new byte[BUFFERSIZE];
        private int receivePos = 0;
        private bool waitForCommand = true;
        private string reqSendChangeEvent = "";
        private bool stopViewChangedEvent = false;
        private DispatcherTimer drawTimer = null;
        private DispatcherTimer sendTimer = null;
        private System.Diagnostics.Stopwatch stopwatch = null;
        private int retryCount = 0;

        private Rectangle backGroundRect = null;

        //------------------
        public MainWindow()
        {
            for (int i = 0; i < 5; i++)
            {
                saveData.ToneSettings[i] = new ToneSetting();
            }

            InitializeComponent();
            DataContext = this;

            serialPort.BaudRate = 115200;
            serialPort.Parity = Parity.None;
            serialPort.DataBits = 8;
            serialPort.StopBits = StopBits.One;
            serialPort.Handshake = Handshake.None;
            serialPort.DataReceived += SerialPort_DataReceived;
            serialPort.ErrorReceived += (sender, e) => { MessageBox.Show("serialError" + e.EventType.ToString()); };

            drawTimer = new DispatcherTimer(DispatcherPriority.ContextIdle, this.Dispatcher);
            drawTimer.Interval = new TimeSpan(0, 0, 0, 0, 200);
            drawTimer.Tick += DispatcherTimer_Tick;
            drawTimer.Start();

            stopwatch = new System.Diagnostics.Stopwatch();
            stopwatch.Start();

            sendTimer = new DispatcherTimer(DispatcherPriority.ContextIdle, this.Dispatcher);
            sendTimer.Interval = new TimeSpan(0, 0, 0, 0, 8);
            sendTimer.Tick += SendTimer_Tick;
            sendTimer.Start();
        }

        //------------------
        private void SendTimer_Tick(object sender, EventArgs e)
        {
            if (commandQueue.Count == 0)
            {
                SyncLabel.Visibility = Visibility.Hidden;
                return;
            }
            long t = stopwatch.ElapsedMilliseconds;
            SyncLabel.Visibility = Visibility.Visible;
            var firstCommand = commandQueue.First();
            if (firstCommand.sended == true)
            {
                long p = t - firstCommand.sendTime;
                if (p < 500)
                {
                    return;
                }
                retryCount++;
                if (retryCount >= 10)
                {
                    if (retryCount == 10)
                    {
                        serialPort.Close();
                        SerialPortComboBox.Text = "";
                        WriteLog("[FAIL] 0x" + String.Format("{0:X2}", firstCommand.sendBuffer[0]));
                        WriteLog(serialPort.PortName + " was disconnected.");
                    }
                    return;
                }
                WriteLog("[RETRY] 0x" + String.Format("{0:X2}", firstCommand.sendBuffer[0]));
            } else
            {
                WriteLog("[SEND] 0x" + String.Format("{0:X2}", firstCommand.sendBuffer[0]));
            }
            firstCommand.sended = true;
            firstCommand.sendTime = t;
            serialPort.Write(firstCommand.sendBuffer, 0, firstCommand.sendBuffer.Length);
        }

        //------------------
        private void RemoveFirstCommand()
        {
            var firstCommand = commandQueue.First();
            if (firstCommand.sended == false) return;

            commandQueue.Remove(firstCommand);
        }

        //------------------
        private SerialCommand AddCommandQueue(Byte command)
        {
            SerialCommand addCommand = new SerialCommand();
            addCommand.SetBufferSize(2);
            addCommand.sendBuffer[0] = command;
            addCommand.sendBuffer[1] = 0xFF;
            commandQueue.Add(addCommand);
            return addCommand;
        }

        //------------------
        private SerialCommand AddCommandQueue(Byte command, Byte param1)
        {
            SerialCommand addCommand = new SerialCommand();
            addCommand.SetBufferSize(3);
            addCommand.sendBuffer[0] = command;
            addCommand.sendBuffer[1] = param1;
            addCommand.sendBuffer[2] = 0xFF;
            commandQueue.Add(addCommand);
            return addCommand;
        }

        //------------------
        private SerialCommand AddCommandQueue(Byte command, Byte param1, Byte param2)
        {
            SerialCommand addCommand = new SerialCommand();
            addCommand.SetBufferSize(4);
            addCommand.sendBuffer[0] = command;
            addCommand.sendBuffer[1] = param1;
            addCommand.sendBuffer[2] = param2;
            addCommand.sendBuffer[3] = 0xFF;
            commandQueue.Add(addCommand);
            return addCommand;
        }

        //------------------
        private SerialCommand AddCommandQueue(Byte command, Byte param1, Byte param2, Byte param3)
        {
            SerialCommand addCommand = new SerialCommand();
            addCommand.SetBufferSize(5);
            addCommand.sendBuffer[0] = command;
            addCommand.sendBuffer[1] = param1;
            addCommand.sendBuffer[2] = param2;
            addCommand.sendBuffer[3] = param3;
            addCommand.sendBuffer[4] = 0xFF;
            commandQueue.Add(addCommand);
            return addCommand;
        }

        //------------------
        private SerialCommand AddCommandQueue(Byte command, Byte param1, Byte param2, Byte param3, Byte param4)
        {
            SerialCommand addCommand = new SerialCommand();
            addCommand.SetBufferSize(6);
            addCommand.sendBuffer[0] = command;
            addCommand.sendBuffer[1] = param1;
            addCommand.sendBuffer[2] = param2;
            addCommand.sendBuffer[3] = param3;
            addCommand.sendBuffer[4] = param4;
            addCommand.sendBuffer[5] = 0xFF;
            commandQueue.Add(addCommand);
            return addCommand;
        }

        //------------------
        private SerialCommand AddCommandQueue(Byte command, Byte[] data)
        {
            SerialCommand addCommand = new SerialCommand();
            addCommand.SetBufferSize(2 + data.Length);
            addCommand.sendBuffer[0] = command;
            for (int i = 0; i < data.Length; i++)
            {
                addCommand.sendBuffer[1 + i] = data[i];
            }
            addCommand.sendBuffer[data.Length + 1] = 0xFF;
            commandQueue.Add(addCommand);
            return addCommand;
        }

        //------------------
        private void DispatcherTimer_Tick(object sender, EventArgs e)
        {
            int toneNo = saveData.GlobalSetting.ToneNo;
            if (WaveCanvas.Children.Count > 0)
            {
                double w = WaveCanvas.ActualWidth - 20;
                double h = WaveCanvas.ActualHeight - 20;
                int i = 0;
                var waveA = saveData.ToneSettings[toneNo].WaveA;
                var waveB = saveData.ToneSettings[toneNo].WaveB;
                foreach (var item in WaveCanvas.Children)
                {
                    if (item.GetType() == typeof(Line))
                    {
                        if (i > 1)
                        {
                            int j = i - 2;
                            Line line = item as Line;
                            double x = 10 + ((j % 256) * 2);
                            double y1 = 0;
                            double y2 = 0;
                            if (j < 256)
                            {
                                y1 = 10 + h / 2 - (h / 2) * (waveA[(j % 256)] / 32767.0);
                                y2 = 10 + h / 2 - (h / 2) * (waveA[((j + 1) % 256)] / 32767.0);
                            } else
                            {
                                y1 = 10 + h / 2 - (h / 2) * (waveB[((j - 256) % 256)] / 32767.0);
                                y2 = 10 + h / 2 - (h / 2) * (waveB[(((j - 256) + 1) % 256)] / 32767.0);
                            }

                            line.X1 = x;
                            line.X2 = x + 2;
                            line.Y1 = y1;
                            line.Y2 = y2;
                        }

                        i++;
                    }
                }
            }
            if (CurveCanvas.Children.Count > 0)
            {
                double w = CurveCanvas.ActualWidth - 20;
                double h = CurveCanvas.ActualHeight - 20;
                int i = 0;
                Brush lineBrush = null;
                var curveTable = saveData.ToneSettings[toneNo].ShiftTable;

                if (RadioButtonToneShift.IsChecked == true)
                {
                    backGroundRect.Fill = new SolidColorBrush(Color.FromArgb(255, 50, 30, 30));
                    lineBrush = Brushes.Red;
                    CurveLabelUp.Content = "WaveB ↑";
                    CurveLabelDown.Content = "WaveA ↓";
                    CurveLabelRight.Content = "→ Volume";
                }
                if (RadioButtonNoise.IsChecked == true)
                {
                    backGroundRect.Fill = new SolidColorBrush(Color.FromArgb(255, 30, 50, 30));
                    lineBrush = Brushes.Lime;
                    CurveLabelUp.Content = "Noisy ↑";
                    CurveLabelDown.Content = "Silent ↓";
                    CurveLabelRight.Content = "→ Volume";
                    curveTable = saveData.ToneSettings[toneNo].NoiseTable;
                }
                if (RadioButtonPitch.IsChecked == true)
                {
                    backGroundRect.Fill = new SolidColorBrush(Color.FromArgb(255, 50, 30, 50));
                    lineBrush = Brushes.Pink;
                    CurveLabelUp.Content = "Pitch Up ↑";
                    CurveLabelDown.Content = "Pitch Down ↓";
                    CurveLabelRight.Content = "→ Volume";
                    curveTable = saveData.ToneSettings[toneNo].PitchTable;
                }

                foreach (var item in CurveCanvas.Children)
                {
                    if (item.GetType() == typeof(Line))
                    {
                        if (i > 1)
                        {
                            int j = i - 2;
                            Line line = item as Line;
                            line.Stroke = lineBrush;
                            double x = 10 + ((j % 32) * (512.0/31.0));
                            double y1 = 10 + h - h * curveTable[j] / 127.0;
                            double y2 = 10 + h - h * curveTable[j + 1] / 127.0;
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
            LogTextBox.Clear();
            if (SerialPortComboBox.SelectedIndex < 0) return;
            var port = (SerialPortComboBox.SelectedItem as string);
            if (port.StartsWith("COM"))
            {
                try
                {
                    serialPort.PortName = port;
                    serialPort.Open();
                    retryCount = 0;

                    if (serialPort.IsOpen)
                    {
                        CheckVersion();
                        SynchronizeAfuueAll();
                    }
                    else
                    {
                        WriteLog("Can't connect to " + port);
                    }
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
                double w = WaveCanvas.ActualWidth - 20;
                double h = WaveCanvas.ActualHeight - 20;
                Rectangle rect = new Rectangle();
                rect.Width = w + 20;
                rect.Height = h + 20;
                rect.Fill = new SolidColorBrush(Color.FromArgb(255, 50, 50, 30));
                WaveCanvas.Children.Add(rect);
                Brush waveLineBrush = new SolidColorBrush(Color.FromArgb(255, 80, 80, 50));
                {
                    Line line = new Line();
                    line.X1 = 10;
                    line.X2 = 10 + w;
                    line.Y1 = line.Y2 = 10 + (h / 2);
                    line.Stroke = waveLineBrush;
                    WaveCanvas.Children.Add(line);
                }
                {
                    Line line = new Line();
                    line.X1 = line.X2 = 10 + (w / 2);
                    line.Y1 = 10;
                    line.Y2 = h + 10;
                    line.Stroke = waveLineBrush;
                    WaveCanvas.Children.Add(line);
                }

                for (int i = 0; i < 256; i++)
                {
                    Line box = new Line();
                    box.Stroke = new SolidColorBrush(Color.FromArgb(255, 255, 192, 32));
                    WaveCanvas.Children.Add(box);
                }
                for (int i = 0; i < 256; i++)
                {
                    Line waveLine = new Line();
                    waveLine.Stroke = new SolidColorBrush(Color.FromArgb(255, 32, 192, 255));
                    WaveCanvas.Children.Add(waveLine);
                }
            }
            {
                double w = CurveCanvas.ActualWidth - 20;
                double h = CurveCanvas.ActualHeight - 20;
                backGroundRect = new Rectangle();
                backGroundRect.Width = w + 20;
                backGroundRect.Height = h + 20;
                backGroundRect.Fill = new SolidColorBrush(Color.FromArgb(255, 50, 30, 30));
                CurveCanvas.Children.Add(backGroundRect);

                {
                    Line line = new Line();
                    line.X1 = 10;
                    line.X2 = 10 + w;
                    line.Y1 = 10 + h;
                    line.Y2 = 10;
                    line.Stroke = new SolidColorBrush(Color.FromArgb(255, 30, 30, 30));
                    CurveCanvas.Children.Add(line);
                }
                {
                    Line line = new Line();
                    line.X1 = 10;
                    line.X2 = 10 + w;
                    line.Y1 = 10 + h/2;
                    line.Y2 = 10 + h/2;
                    line.Stroke = new SolidColorBrush(Color.FromArgb(255, 30, 30, 30));
                    CurveCanvas.Children.Add(line);
                }

                for (int i = 0; i < 32; i++)
                {
                    if (i == 31) break;

                    Line shiftLine = new Line();
                    shiftLine.Stroke = new SolidColorBrush(Color.FromArgb(255, 130, 192, 255));
                    CurveCanvas.Children.Add(shiftLine);
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
            string[] voiceList =
            {
                "1: Acoustic Piano",
                "2: Bright Piano",
                "3: Electric Grand Piano",
                "4: Honky-tonk Piano",
                "5: Electric Piano",
                "6: Electric Piano 2",
                "7: Harpsichord",
                "8: Clavi",
                "9: Celesta",
                "10:  Glockenspiel",
                "11: Musical box",
                "12: Vibraphone",
                "13: Marimba",
                "14: Xylophone",
                "15: Tubular Bell",
                "16: Dulcime",
                "17: Drawbar Organ",
                "18: Percussive Organ",
                "19: Rock Organ",
                "20:  Church organ",
                "21: Reed organ",
                "22: Accordion",
                "23: Harmonica",
                "24: Tango Accordion",
                "25: Acoustic Guitar (nylon)",
                "26: Acoustic Guitar (steel)",
                "27: Electric Guitar (jazz)",
                "28: Electric Guitar (clean)",
                "29: Electric Guitar (muted)",
                "30:  Overdriven Guitar",
                "31: Distortion Guitar",
                "32: Guitar harmonics",
                "33: Acoustic Bass",
                "34: Electric Bass (finger)",
                "35: Electric Bass (pick)",
                "36: Fretless Bass",
                "37: Slap Bass 1",
                "38: Slap Bass 2",
                "39: Synth Bass 1",
                "40:  Synth Bass 2",
                "41: Violin",
                "42: Viola",
                "43: Cello",
                "44: Double Bass",
                "45: Tremolo Strings",
                "46: Pizzicato Strings",
                "47: Orchestral Harp",
                "48: Timpani",
                "49: String Ensemble 1",
                "50:  String Ensemble 2",
                "51: Synth Strings 1",
                "52: Synth Strings 2",
                "53: Voice Aahs",
                "54: Voice Oohs",
                "55: Synth Voice",
                "56: Orchestra Hit",
                "57: Trumpet",
                "58: Trombone",
                "59: Tuba",
                "60: Muted Trumpet",
                "61: French horn",
                "62: Brass Section",
                "63: Synth Brass 1",
                "64: Synth Brass 2",
                "65: Soprano Sax",
                "66: Alto Sax",
                "67: Tenor Sax",
                "68: Baritone Sax",
                "69: Oboe",
                "70:  English Horn",
                "71: Bassoon",
                "72: Clarinet",
                "73: Piccolo",
                "74: Flute",
                "75: Recorder",
                "76: Pan Flute",
                "77: Blown Bottle",
                "78: Shakuhachi",
                "79: Whistle",
                "80:  Ocarina",
                "81: Lead 1 (square)",
                "82: Lead 2 (sawtooth)",
                "83: Lead 3 (calliope)",
                "84: Lead 4 (chiff)",
                "85: Lead 5 (charang)",
                "86: Lead 6 (voice)",
                "87: Lead 7 (fifths)",
                "88: Lead 8 (bass + lead)",
                "89: Pad 1 (Fantasia)",
                "90:  Pad 2 (warm)",
                "91: Pad 3 (polysynth)",
                "92: Pad 4 (choir)",
                "93: Pad 5 (bowed)",
                "94: Pad 6 (metallic)",
                "95: Pad 7 (halo)",
                "96: Pad 8 (sweep)",
                "97: FX 1 (rain)",
                "98: FX 2 (soundtrack)",
                "99: FX 3 (crystal)",
                "100: FX 4 (atmosphere)",
                "101: FX 5 (brightness)",
                "102: FX 6 (goblins)",
                "103: FX 7 (echoes)",
                "104: FX 8 (sci-fi)",
                "105: Sitar",
                "106: Banjo",
                "107: Shamisen",
                "108: Koto",
                "109: Kalimba",
                "110: Bagpipe",
                "111: Fiddle",
                "112: Shanai",
                "113: Tinkle Bell",
                "114: Agogo",
                "115: Steel Drums",
                "116: Woodblock",
                "117: Taiko Drum",
                "118: Melodic Tom",
                "119: Synth Drum",
                "120: Reverse Cymbal",
                "121: Guitar Fret Noise",
                "122: Breath Noise",
                "123: Seashore",
                "124: Bird Tweet",
                "125: Telephone Ring",
                "126: Helicopter",
                "127: Applause",
                "128: Gunshot",
            };
            foreach (var s in voiceList)
            {
                MidiPgNoComboBox.Items.Add(s);
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
             * 0xA8 SET METRONOME
             * 0xA9 SET MIDI
             * 0xAA SET WAVE A DATA
             * 0xAB SET WAVE B DATA
             * 0xAC SET TONE SHIFT CURVE TABLE
             * 0xAD SET NOISECURVE TABLE
             * 0xAE SET PITCH CURVE TABLE
             * 
             * 0xB1 GET TONE NUMBER
             * 0xB2 GET TRANSPOSE
             * 0xB3 GET FINE TUNE
             * 0xB4 GET REVERB LEVEL
             * 0xB5 GET PORTAMENT LEVEL
             * 0xB6 GET KEY SENSITIVITY
             * 0xB7 GET BREATH SENSITIVITY
             * 0xB8 GET METRONOME
             * 0xB9 GET MIDI
             * 0xBA GET WAVE A DATA
             * 0xBB GET WAVE B DATA
             * 0xBC GET TONE SHIFT CURVE TABLE
             * 0xBD GET NOISECURVE TABLE
             * 0xBE GET PITCH CURVE TABLE
             * 
             * 0xC1 RESPONSE TONE NUMBER
             * 0xC2 RESPONSE TRANSPOSE
             * 0xC3 RESPONSE FINE TUNE
             * 0xC4 RESPONSE REVERB LEVEL
             * 0xC5 RESPONSE PORTAMENT LEVEL
             * 0xC6 RESPONSE KEY SENSITIVITY
             * 0xC7 RESPONSE BREATH SENSITIVITY
             * 0xC8 RESPONSE METRONOME
             * 0xC9 RESPONSE MIDI
             * 0xCA RESPONSE WAVE A DATA
             * 0xCB RESPONSE WAVE B DATA
             * 0xBC RESPONSE TONE SHIFT CURVE TABLE
             * 0xBD RESPONSE NOISECURVE TABLE
             * 0xBE RESPONSE PITCH CURVE TABLE
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
                        int tno = receiveBuffer[1];
                        int vol0 = receiveBuffer[2];
                        int vol1 = receiveBuffer[3];
                        int vol2 = receiveBuffer[4];
                        this.Dispatcher.Invoke((Action)(() =>
                        {
                            saveData.GlobalSetting.ToneNo = (Byte)tno;
                            ToneComboBox.SelectedIndex = tno;
                            CheckBoxEnableChordPlay.IsChecked = (vol1 > 0) || (vol2 > 0);
                            RemoveFirstCommand();
                        }));
                    }
                    break;
                case 0xA2: // SET TRANSPOSE
                case 0xC2: // RESPONSE TRANSPOSE
                    {
                        int tno = receiveBuffer[1];
                        int data = ChangeSigned(receiveBuffer[2]);
                        this.Dispatcher.Invoke((Action)(() =>
                        {
                            saveData.ToneSettings[tno].Transpose = (SByte)data;
                            TransposeComboBox.SelectedIndex = data + 12;
                            RemoveFirstCommand();
                        }));
                    }
                    break;
                case 0xA3: // SET FINE TUNE
                case 0xC3: // RESPONSE FINE TUNE
                    {
                        int tno = receiveBuffer[1];
                        int data = ChangeSigned(receiveBuffer[2]);
                        this.Dispatcher.Invoke((Action)(() =>
                        {
                            saveData.ToneSettings[tno].FineTune = (SByte)data;
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
                            RemoveFirstCommand();
                        }));
                    }
                    break;
                case 0xA4: // SET REVERB LEVEL
                case 0xC4: // RESPONSE REVERB LEVEL
                    {
                        int tno = receiveBuffer[1];
                        int data = ChangeSigned(receiveBuffer[2]);
                        this.Dispatcher.Invoke((Action)(() =>
                        {
                            saveData.ToneSettings[tno].Reverb = (Byte)data;
                            ReverbLevelSlider.Value = data;
                            RemoveFirstCommand();
                        }));
                    }
                    break;
                case 0xA5: // SET PORTAMENT LEVEL
                case 0xC5: // RESPONSE PORTAMENT LEVEL
                    {
                        int tno = receiveBuffer[1];
                        int data = ChangeSigned(receiveBuffer[2]);
                        this.Dispatcher.Invoke((Action)(() =>
                        {
                            saveData.ToneSettings[tno].Portamento = (Byte)data;
                            PortamentoLevelSlider.Value = data;
                            RemoveFirstCommand();
                        }));
                    }
                    break;
                case 0xA6: // SET KEY SENSITIVITY
                case 0xC6: // RESPONSE KEY SENSITIVITY
                    {
                        int tno = receiveBuffer[1];
                        int data = ChangeSigned(receiveBuffer[2]);
                        this.Dispatcher.Invoke((Action)(() =>
                        {
                            saveData.ToneSettings[tno].KeySense = (Byte)data;
                            KeySensitivityLevelSlider.Value = data;
                            RemoveFirstCommand();
                        }));
                    }
                    break;
                case 0xA7: // SET BREATH SENSITIVITY
                case 0xC7: // RESPONSE BREATH SENSITIVITY
                    {
                        int tno = receiveBuffer[1];
                        int data = ChangeSigned(receiveBuffer[2]);
                        this.Dispatcher.Invoke((Action)(() =>
                        {
                            saveData.ToneSettings[tno].BreathSense = (Byte)data;
                            BreathSensitivityLevelSlider.Value = data;
                            RemoveFirstCommand();
                        }));
                    }
                    break;
                case 0xA8: // SET METRONOME
                case 0xC8: // RESPONSE METRONOME
                    {
                        Byte enabled = receiveBuffer[1];
                        Byte mode = receiveBuffer[2];
                        Byte vol = receiveBuffer[3];
                        int d0 = receiveBuffer[4];
                        int d1 = receiveBuffer[5];
                        this.Dispatcher.Invoke((Action)(() =>
                        {
                            CheckBoxMetronomeEnable.IsChecked = (enabled == 1);
                            saveData.GlobalSetting.Metronome_m = mode;
                            saveData.GlobalSetting.Metronome_v = vol;
                            int tempo = d0 | (d1 << 7);
                            saveData.GlobalSetting.Metronome_t = tempo;
                            RadioButtonMetronomeSimple.IsChecked = (mode == 1);
                            RadioButtonMetronome2Beats.IsChecked = (mode == 2);
                            RadioButtonMetronome3Beats.IsChecked = (mode == 3);
                            RadioButtonMetronome4Beats.IsChecked = (mode == 4);
                            MetronomeVolumeSlider.Value = vol;
                            MetronomeTempoSlider.Value = tempo;
                            RemoveFirstCommand();
                        }));
                    }
                    break;
                case 0xA9: // SET MIDI
                case 0xC9: // RESPONSE MIDI
                    {
                        Byte tp = receiveBuffer[1];
                        Byte pg = receiveBuffer[2];
                        this.Dispatcher.Invoke((Action)(() =>
                        {
                            saveData.GlobalSetting.MidiControlType = tp;
                            RadioButtonBC.IsChecked = (tp == 1);
                            RadioButtonExp.IsChecked = (tp == 2);
                            RadioButtonAft.IsChecked = (tp == 3);
                            RadioButtonVol.IsChecked = (tp == 4);
                            saveData.GlobalSetting.MidiPgNo = pg;
                            MidiPgNoComboBox.SelectedIndex = pg;
                            RemoveFirstCommand();
                        }));
                    }
                    break;
                case 0xAA: // SET WAVE A DATA
                case 0xCA: // RESPONSE WAVE A DATA
                case 0xAB: // SET WAVE B DATA
                case 0xCB: // RESPONSE WAVE B DATA
                    if (receivePos >= 256) {
                        int tno = receiveBuffer[1];
                        bool isWaveB = false;
                        if ((command & 0x0F) == 0x0B)
                        {
                            isWaveB = true;
                        }
                        for (int i = 0; i < 256; i++)
                        {
                            int d0 = receiveBuffer[2 + i * 2];
                            int d1 = receiveBuffer[2 + i * 2 + 1];
                            int data = (d0 << 2) | (d1 << 9);
                            if (data >= 0x8000)
                            {
                                data = data - 0x10000;
                            }
                            if (isWaveB == false)
                            {
                                saveData.ToneSettings[tno].WaveA[i] = (Int16)data;
                            } else
                            {
                                saveData.ToneSettings[tno].WaveB[i] = (Int16)data;
                            }
                        }
                        RemoveFirstCommand();
                    }
                    break;
                case 0xAC: // SET TONE SHIFT CURVE TABLE
                case 0xCC: // RESPONSE TONE SHIFT CURVE TABLE
                case 0xAD: // SET NOISE CURVE TABLE
                case 0xCD: // RESPONSE NOISE CURVE TABLE
                case 0xAE: // SET PITCH CURVE TABLE
                case 0xCE: // RESPONSE PITCH CURVE TABLE
                    if (receivePos >= 32)
                    {
                        int tno = receiveBuffer[1];
                        for (int i = 0; i < 32; i++)
                        {
                            if ((command & 0x0F) == 0x0C)
                            {
                                saveData.ToneSettings[tno].ShiftTable[i] = receiveBuffer[2 + i];
                            }
                            else if ((command & 0x0F) == 0x0D)
                            {
                                saveData.ToneSettings[tno].NoiseTable[i] = receiveBuffer[2 + i];
                            }
                            else if ((command & 0x0F) == 0x0E)
                            {
                                saveData.ToneSettings[tno].PitchTable[i] = receiveBuffer[2 + i];
                            }
                        }
                        RemoveFirstCommand();
                    }
                    break;
                case 0xF1: // GET VERSION
                    {
                        int ver = receiveBuffer[1];
                        int protocol = receiveBuffer[2];
                        if ((ver == commVer1) && (protocol == commVer2))
                        {
                            this.Dispatcher.Invoke((Action)(() =>
                            {
                                CheckLabel.Visibility = Visibility.Collapsed;
                                RemoveFirstCommand();
                            }));
                        }
                    }
                    break;
                case 0xFE: // COMMAND ACK
                    {
                        WriteLog("(ACK)");
                        var firstCommand = commandQueue.First();
                        bool sended = firstCommand.sended;
                        Action ackAction = firstCommand.ackAction;
                        RemoveFirstCommand();

                        if ((sended == true) && (ackAction != null))
                        {
                            ackAction();
                        }
                    }
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
        private SerialCommand GetToneNumber()
        {
            if (serialPort.IsOpen == false) return null;

            WriteLog("send get tone number");
            return AddCommandQueue(0xB1); // GET TONE NUMBER
        }

        //------------------
        private void SendToneNumber(int toneNo)
        {
            if (serialPort.IsOpen == false) return;

            WriteLog("send set tone number");
            Byte vol0 = 127;
            Byte vol1 = 0;
            Byte vol2 = 0;
            if (CheckBoxEnableChordPlay.IsChecked == true)
            {
                vol0 = vol1 = vol2 = 64;
            }
            AddCommandQueue(0xA1, (Byte)toneNo, vol0, vol1, vol2);  // SET TONE NUMBER
        }

        //------------------
        private SerialCommand GetGlobalSettings()
        {
            if (serialPort.IsOpen == false) return null;

            WriteLog("send get global settings");
            AddCommandQueue(0xB9); // GET MIDI
            return AddCommandQueue(0xB8); // GET METRONOME
        }

        //------------------
        private void SendGlobalSettings()
        {
            if (serialPort.IsOpen == false) return;

            WriteLog("send set global settings");
            int tempo = saveData.GlobalSetting.Metronome_t;
            byte[] data = new byte[5];
            data[0] = 0;
            if (CheckBoxMetronomeEnable.IsChecked == true)
            {
                data[0] = 1;
            }
            data[1] = (Byte)saveData.GlobalSetting.Metronome_m;
            data[2] = (Byte)saveData.GlobalSetting.Metronome_v;
            data[3] = (Byte)(tempo & 0x7F);
            data[4] = (Byte)((tempo >> 7) & 0x7F);
            AddCommandQueue(0xA8, data); // SET METRONOME
        }

        //------------------
        private void SendToneData(int toneNo)
        {
            if (serialPort.IsOpen == false) return;

            var ts = saveData.ToneSettings[toneNo];
            WriteLog("send set all");
            AddCommandQueue(0xA2, (Byte)toneNo, (Byte)ChangeUnsigned(ts.Transpose)); // SET TRANSPOSE
            AddCommandQueue(0xA3, (Byte)toneNo, (Byte)ChangeUnsigned(ts.FineTune)); // SET FINE TUNE
            AddCommandQueue(0xA4, (Byte)toneNo, (Byte)ts.Reverb); // SET REVERB LEVEL
            AddCommandQueue(0xA5, (Byte)toneNo, (Byte)ts.Portamento); // SET PORTAMENTO LEVEL
            AddCommandQueue(0xA6, (Byte)toneNo, (Byte)ts.KeySense); // SET KEY SENSITIVITY
            AddCommandQueue(0xA7, (Byte)toneNo, (Byte)ts.BreathSense); // SET BREATH SENSITIVIY

            SendWaveData(toneNo, false);
            SendWaveData(toneNo, true);

            SendCurveTable(toneNo, 0);
            SendCurveTable(toneNo, 1);
            SendCurveTable(toneNo, 2);
        }

        //------------------
        private SerialCommand GetToneData(int toneNo)
        {
            if (serialPort.IsOpen == false) return null;

            WriteLog("send get all");
            AddCommandQueue(0xB2, (Byte)toneNo); // GET TRANSPOSE
            AddCommandQueue(0xB3, (Byte)toneNo); // GET FINE TUNE
            AddCommandQueue(0xB4, (Byte)toneNo); // GET REVERB LEVEL
            AddCommandQueue(0xB5, (Byte)toneNo); // GET PORTAMENT LEVEL
            AddCommandQueue(0xB6, (Byte)toneNo); // GET KEY SENSITIVITY
            AddCommandQueue(0xB7, (Byte)toneNo); // GET BREATH SENSITIVIY

            AddCommandQueue(0xBA, (Byte)toneNo); // GET WAVE A DATA
            AddCommandQueue(0xBB, (Byte)toneNo); // GET WAVE B DATA
            AddCommandQueue(0xBC, (Byte)toneNo); // GET SHIFT TABLE
            AddCommandQueue(0xBD, (Byte)toneNo); // GET NOISE TABLE
            return AddCommandQueue(0xBE, (Byte)toneNo); // GET PITCH TABLE
        }

        //------------------
        private void SendWaveData(int toneNo, bool isWaveB)
        {
            if (serialPort.IsOpen == false) return;

            var ts = saveData.ToneSettings[toneNo];

            byte[] buff = new byte[513];
            buff[0] = (Byte)toneNo;
            for (int i = 0; i < 256; i++)
            {
                int data = 0;
                if (isWaveB == false)
                {
                    data = ts.WaveA[i];
                }
                else
                {
                    data = ts.WaveB[i];
                }
                if (data < 0)
                {
                    data = 0x10000 + data;
                }
                int d0 = (data >> 2) & 0x7F;
                int d1 = (data >> 9) & 0x7F;
                buff[1 + i * 2] = (byte)(d0);
                buff[1 + i * 2 + 1] = (byte)(d1);
            }

            if (isWaveB == false)
            {
                WriteLog("send set wave a data");
                AddCommandQueue(0xAA, buff); // SET WAVE A DATA
            }
            else
            {
                WriteLog("send set wave b data");
                AddCommandQueue(0xAB, buff); // SET WAVE B DATA
            }
        }

        //------------------
        private void SendCurveTable(int toneNo, int mode)
        {
            if (serialPort.IsOpen == false) return;

            bool isToneShiftCurve = (mode == 0);
            bool isNoiseCurve = (mode == 1);
            bool isPitchCurve = (mode == 2);

            var ts = saveData.ToneSettings[toneNo];

            byte[] buff = new byte[33];
            buff[0] = (byte)toneNo;
            for (int i = 0; i < 32; i++)
            {
                int data = 0;
                if (isToneShiftCurve)
                {
                    data = (int)(ts.ShiftTable[i]);
                }
                else if (isNoiseCurve)
                {
                    data = (int)(ts.NoiseTable[i]);
                }
                else if (isPitchCurve)
                {
                    data = (int)(ts.PitchTable[i]);
                }
                if (data > 127) data = 127;
                if (data < 0)
                {
                    data = 0;
                }
                buff[1 + i] = (byte)(data);
            }

            if (isToneShiftCurve)
            {
                WriteLog("send shift table");
                AddCommandQueue(0xAC, buff); // SET SHIFT TABLE
            }
            else if (isNoiseCurve)
            {
                WriteLog("send noise table");
                AddCommandQueue(0xAD, buff); // SET NOISE TABLE
            }
            else if (isPitchCurve)
            {
                WriteLog("send pitch table");
                AddCommandQueue(0xAE, buff); // SET PITCH TABLE
            }
        }

        //------------------
        private void WriteButton_Click(object sender, RoutedEventArgs e)
        {
            if (serialPort.IsOpen == false) return;

            WriteLog("send flash write");
            AddCommandQueue(0xE1, 0x7F);    // WRITE TO FLASH
        }

        //------------------
        private void ToneComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (serialPort.IsOpen == false) return;
            if (stopViewChangedEvent) return;
            saveData.GlobalSetting.ToneNo = (Byte)ToneComboBox.SelectedIndex;
            SendToneNumber(saveData.GlobalSetting.ToneNo);
            GetToneData(saveData.GlobalSetting.ToneNo);
        }

        //------------------
        private void SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (serialPort.IsOpen == false) return;
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
        private void ComboBoxChanged(object sender, SelectionChangedEventArgs e)
        {
            if (serialPort.IsOpen == false) return;
            if (stopViewChangedEvent) return;

            byte[] buff = new Byte[256];
            if (sender == TransposeComboBox)
            {
                WriteLog("send set transpose");
                int data = TransposeComboBox.SelectedIndex - 12;
                saveData.ToneSettings[saveData.GlobalSetting.ToneNo].Transpose = (SByte)data;
                AddCommandQueue(0xA2, (Byte)saveData.GlobalSetting.ToneNo, (Byte)ChangeUnsigned(data)); // SET TRANSPOSE
            }
            if (sender == TuneComboBox)
            {
                WriteLog("send set finetune");
                Label label = TuneComboBox.SelectedItem as Label;
                string[] tunestr = (label.Content as string).Split(' ');
                int tune = int.Parse(tunestr[0]) - 440;
                saveData.ToneSettings[saveData.GlobalSetting.ToneNo].FineTune = (SByte)tune;
                AddCommandQueue(0xA3, (Byte)saveData.GlobalSetting.ToneNo, (Byte)ChangeUnsigned(tune)); // SET FINE TUNE
            }
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

            bool isWaveB = (bool)RadioButtonWaveB.IsChecked;
            double w = WaveCanvas.ActualWidth;
            double h = WaveCanvas.ActualHeight;

            Point p = e.GetPosition(WaveCanvas);
            double mx = ((Point)waveCanvasMousePoint).X;
            double my = ((Point)waveCanvasMousePoint).Y;
            double dx = p.X - mx;
            double dy = p.Y - my;

            for (int i = 0; i < 256; i++)
            {
                double x = i * w / 256.0;
                if ( ((mx <= x) && (x <= p.X)) || ((p.X <= x) && (x <= mx)) )
                {
                    double v = -((my + (dy * i) / 256) - h / 2) / 127.0;
                    if (v > 1.0) v = 1.0;
                    if (v < -1.0) v = -1.0;
                    if (isWaveB == false)
                    {
                        saveData.ToneSettings[saveData.GlobalSetting.ToneNo].WaveA[i] = (Int16)(v * 32767);
                    } else
                    {
                        saveData.ToneSettings[saveData.GlobalSetting.ToneNo].WaveB[i] = (Int16)(v * 32767);
                    }
                }
            }
            waveCanvasMousePoint = p;
        }

        //------------------
        private void WaveCanvas_MouseUp(object sender, MouseButtonEventArgs e)
        {
            waveCanvasMousePoint = null;
            WaveCanvas.ReleaseMouseCapture();
            SendWaveData(saveData.GlobalSetting.ToneNo, (bool)RadioButtonWaveB.IsChecked);
        }

        private Point? curveCanvasMousePoint = null;
        //------------------
        private void ShiftCanvas_MouseDown(object sender, MouseButtonEventArgs e)
        {
            curveCanvasMousePoint = e.GetPosition(CurveCanvas);
            CurveCanvas.CaptureMouse();
        }

        //------------------
        private void ShiftCanvas_MouseMove(object sender, MouseEventArgs e)
        {
            if (curveCanvasMousePoint == null) return;

            double w = CurveCanvas.ActualWidth;
            double h = CurveCanvas.ActualHeight;

            Point p = e.GetPosition(CurveCanvas);
            p.X -= 10;
            p.Y -= 10;
            double mx = ((Point)curveCanvasMousePoint).X;
            double my = ((Point)curveCanvasMousePoint).Y;
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
                    if (RadioButtonToneShift.IsChecked == true)
                    {
                        saveData.ToneSettings[saveData.GlobalSetting.ToneNo].ShiftTable[i] = (Byte)(127 * v);
                    }
                    if (RadioButtonNoise.IsChecked == true)
                    {
                        saveData.ToneSettings[saveData.GlobalSetting.ToneNo].NoiseTable[i] = (Byte)(127 * v);
                    }
                    if (RadioButtonPitch.IsChecked == true)
                    {
                        saveData.ToneSettings[saveData.GlobalSetting.ToneNo].PitchTable[i] = (Byte)(127 * v);
                    }
                }
            }
            curveCanvasMousePoint = p;
        }

        //------------------
        private void ShiftCanvas_MouseUp(object sender, MouseButtonEventArgs e)
        {
            curveCanvasMousePoint = null;
            CurveCanvas.ReleaseMouseCapture();
            int toneNo = saveData.GlobalSetting.ToneNo;
            if ((bool)RadioButtonToneShift.IsChecked)
            {
                SendCurveTable(toneNo, 0);
            }
            if ((bool)RadioButtonNoise.IsChecked)
            {
                SendCurveTable(toneNo, 1);
            }
            if ((bool)RadioButtonPitch.IsChecked)
            {
                SendCurveTable(toneNo, 2);
            }
        }

        //------------------
        private void Slider_MouseDown(object sender, MouseButtonEventArgs e)
        {
        }

        //------------------
        private void Slider_MouseUp(object sender, MouseButtonEventArgs e)
        {
            if (serialPort.IsOpen == false) return;

            if (reqSendChangeEvent != "")
            {
                WriteLog("send " + reqSendChangeEvent);
            }
            int tno = saveData.GlobalSetting.ToneNo;
            if (reqSendChangeEvent == "Reverb")
            {
                saveData.ToneSettings[saveData.GlobalSetting.ToneNo].Reverb = (Byte)ReverbLevelSlider.Value;
                AddCommandQueue(0xA4, (Byte)tno, (Byte)ReverbLevelSlider.Value); // SET REVERB LEVEL
            }
            if (reqSendChangeEvent == "Portamento")
            {
                saveData.ToneSettings[saveData.GlobalSetting.ToneNo].Portamento = (Byte)PortamentoLevelSlider.Value;
                AddCommandQueue(0xA5, (Byte)tno, (Byte)PortamentoLevelSlider.Value); // SET PORTAMENT LEVEL
            }
            if (reqSendChangeEvent == "KeySense")
            {
                saveData.ToneSettings[saveData.GlobalSetting.ToneNo].KeySense = (Byte)KeySensitivityLevelSlider.Value;
                AddCommandQueue(0xA6, (Byte)tno, (Byte)KeySensitivityLevelSlider.Value); // SET KEY SENSITIVITY
            }
            if (reqSendChangeEvent == "BreathSense")
            {
                saveData.ToneSettings[saveData.GlobalSetting.ToneNo].BreathSense = (Byte)BreathSensitivityLevelSlider.Value;
                AddCommandQueue(0xA7, (Byte)tno, (Byte)BreathSensitivityLevelSlider.Value); // SET BREATH SENSITIVIY
            }
            reqSendChangeEvent = "";
        }

        //------------------
        private void SynchronizeAfuueAll()
        {
            GetGlobalSettings();
            for (int i = 1; i < 5; i++)
            {
                GetToneData(i);
            }
            SendToneNumber(0);
            GetToneNumber();
            GetToneData(0);
        }

        //------------------
        private void RebootButton_Click(object sender, RoutedEventArgs e)
        {
            if (serialPort.IsOpen == false) return;

            var command = AddCommandQueue(0xEE, 0x7F);    // REBOOT

            command.ackAction = new Action(() => {
                WriteLog("reboot");
                WriteLog("wait 10 seconds ...");
                System.Threading.Thread.Sleep(7000);

                this.Dispatcher.Invoke((Action)(() =>
                {
                    SynchronizeAfuueAll();
                }));
            });
        }

        //------------------
        private void FactoryResetButton_Click(object sender, RoutedEventArgs e)
        {
            // Are you sure?

            if (serialPort.IsOpen == false) return;

            var command = AddCommandQueue(0xEE, 0x7A, 0x3C);

            command.ackAction = new Action(() => {
                WriteLog("factory reset");
                WriteLog("wait 10 seconds ...");
                System.Threading.Thread.Sleep(7000);

                this.Dispatcher.Invoke((Action)(() =>
                {
                    SynchronizeAfuueAll();
                }));
            });
        }

        //------------------
        private void LogClearButton_Click(object sender, RoutedEventArgs e)
        {
            LogTextBox.Clear();
        }

        //------------------
        private void MenuItemOpen_Click(object sender, RoutedEventArgs e)
        {
            Microsoft.Win32.OpenFileDialog openFileDialog = new Microsoft.Win32.OpenFileDialog();
            openFileDialog.DefaultExt = ".afux";
            var ret = openFileDialog.ShowDialog();

            if (ret == true)
            {
                System.Xml.Serialization.XmlSerializer ser = new System.Xml.Serialization.XmlSerializer(typeof(SaveData));
                using (var sr = new System.IO.StreamReader(openFileDialog.FileName, Encoding.UTF8))
                {
                    saveData = (SaveData)ser.Deserialize(sr);
                    sr.Close();
                }

                for (int i = 0; i < 5; i++)
                {
                    SendToneData(i);
                }

                SendToneNumber(0);
                GetToneNumber();
                GetToneData(0);
            }
        }

        //------------------
        private void MenuItemSaveAs_Click(object sender, RoutedEventArgs e)
        {
            Microsoft.Win32.SaveFileDialog saveFileDialog = new Microsoft.Win32.SaveFileDialog();
            saveFileDialog.DefaultExt = ".afux";
            var ret = saveFileDialog.ShowDialog();

            if (ret == true)
            {
                System.Xml.Serialization.XmlSerializer ser = new System.Xml.Serialization.XmlSerializer(typeof(SaveData));
                using (var sw = new System.IO.StreamWriter(saveFileDialog.FileName, false, Encoding.UTF8))
                {
                    ser.Serialize(sw, saveData);
                    sw.Close();
                }
            }
        }

        //------------------
        private void MetronomeVolumeSlider_MouseUp(object sender, MouseButtonEventArgs e)
        {
            saveData.GlobalSetting.Metronome_v = (Byte)MetronomeVolumeSlider.Value;
            saveData.GlobalSetting.Metronome_t = (int)MetronomeTempoSlider.Value;
            SendGlobalSettings();
        }

        //------------------
        private void MetronomeStatusChanged(object sender, RoutedEventArgs e)
        {
            if (RadioButtonMetronomeSimple.IsChecked == true)
            {
                saveData.GlobalSetting.Metronome_m = 1;
            }
            if (RadioButtonMetronome2Beats.IsChecked == true)
            {
                saveData.GlobalSetting.Metronome_m = 2;
            }
            if (RadioButtonMetronome3Beats.IsChecked == true)
            {
                saveData.GlobalSetting.Metronome_m = 3;
            }
            if (RadioButtonMetronome4Beats.IsChecked == true)
            {
                saveData.GlobalSetting.Metronome_m = 4;
            }
            SendGlobalSettings();
        }

        //------------------
        private void CheckVersion()
        {
            AddCommandQueue(0xF1);
        }

        //------------------
        private void CheckBoxEnableChordPlay_Click(object sender, RoutedEventArgs e)
        {
            SendToneNumber(saveData.GlobalSetting.ToneNo);
        }

        //------------------
        private void MenuItemShowFingeringChart_Click(object sender, RoutedEventArgs e)
        {
            HelpWindow helpWindow = new HelpWindow();
            helpWindow.ShowDialog();
        }

        //------------------
        private void MidiPgNoComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            saveData.GlobalSetting.MidiPgNo = (byte)MidiPgNoComboBox.SelectedIndex;
            AddCommandQueue(0xA9, (byte)saveData.GlobalSetting.MidiControlType, (byte)saveData.GlobalSetting.MidiPgNo);
        }

        //------------------
        private void RadioButtonBC_Click(object sender, RoutedEventArgs e)
        {
            int tp = 0;
            if (RadioButtonBC.IsChecked == true) tp = 1;
            if (RadioButtonExp.IsChecked == true) tp = 2;
            if (RadioButtonAft.IsChecked == true) tp = 3;
            if (RadioButtonVol.IsChecked == true) tp = 4;
            saveData.GlobalSetting.MidiControlType = (byte)tp;
            AddCommandQueue(0xA9, (byte)saveData.GlobalSetting.MidiControlType, (byte)saveData.GlobalSetting.MidiPgNo);
        }
    }
}

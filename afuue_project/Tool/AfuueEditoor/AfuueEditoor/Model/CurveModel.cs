using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace AfuueEditoor.Model
{
    public class CurveModel : INotifyPropertyChanged
    {
        public ObservableCollection<double> Values { get; private set; }
        public int Count { get => Values.Count; }

        public event PropertyChangedEventHandler PropertyChanged;

        private void RaisePropertyChanged(string propertyName)
        {
            if (PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
        }

        public void SetValue(int idx, double value)
        {
            if ((idx < 0) || (idx >= Values.Count))
            {
                return;
            }
            Values[idx] += value;
            Console.WriteLine("ValueChanged " + idx + "," + value +", " + Values[idx]*100);
            RaisePropertyChanged(nameof(Values));
        }

        public CurveModel()
        {
            Values = new ObservableCollection<double>();

            string filename = @".\stepTable.txt";
            if (System.IO.File.Exists(filename))
            {
                string[] lines = System.IO.File.ReadAllText(filename).Split('\n');
                foreach (var line in lines)
                {
                    if (line.Contains("{") || line.Contains("}"))
                    {
                        continue;
                    }
                    string[] s = line.TrimEnd(',').Split(',');
                    Values.Add(double.Parse(s[1]));
                }
            }
        }
    }
}

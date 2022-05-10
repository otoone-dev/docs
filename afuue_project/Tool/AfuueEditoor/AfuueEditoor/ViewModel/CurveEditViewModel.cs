using AfuueEditoor.Model;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using Reactive.Bindings;
using System.Windows.Media;
using System.ComponentModel;

namespace AfuueEditoor.ViewModel
{
    public class CurveEditViewModel : INotifyPropertyChanged
    {
        private ObservableCollection<Point> _points;

        public event PropertyChangedEventHandler PropertyChanged;

        private void RaisePropertyChanged(string propertyName)
        {
            if (PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
        }

        public PointCollection Points {
            get
            {
                return new PointCollection(_points);
            }
        }

        public CurveEditViewModel(CurveModel curve)
        {
            _points = new ObservableCollection<Point>();
            foreach (var v in curve.Values)
            {
                _points.Add(new Point(0, v));
            }
            UpdateRate();
            curve.Values.CollectionChanged += Values_CollectionChanged;
            curve.PropertyChanged += (sender, e) =>
            {
                for (int i = 0; i < curve.Values.Count; i++)
                {
                    var p = _points[i];
                    p.Y = curve.Values[i];
                    _points[i] = p;
                }
                RaisePropertyChanged(nameof(Points));
            };
            ValueChanged = (idx, value) => {
                curve.SetValue(idx, value);
            };
        }

        private void UpdateRate()
        {
            double rate = 1.0 / (_points.Count - 1);
            for (int i = 0; i < _points.Count; i++)
            {
                var p = _points[i];
                p.X = i * rate;
                _points[i] = p;
            }
        }

        private void Values_CollectionChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            if (e.Action == NotifyCollectionChangedAction.Add)
            {
                foreach (var item in e.NewItems)
                {
                    _points.Add(new Point(0, (double)item));
                }
                UpdateRate();
            }
        }

        public Action<int, double> ValueChanged { get; private set; }
    }
}

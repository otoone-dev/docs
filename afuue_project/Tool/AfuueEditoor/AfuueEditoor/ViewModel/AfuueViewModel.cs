using AfuueEditoor.Model;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace AfuueEditoor.ViewModel
{
    public class AfuueViewModel
    {
        public AfuueStorageViewModel AfuueStorageViewModel { get; private set; }
        public CurveEditViewModel CurveEditViewModel { get; private set; }
        public AfuueViewModel()
        {
            AfuueStorageViewModel = new AfuueStorageViewModel();
            CurveEditViewModel = new CurveEditViewModel(new CurveModel());
        }
    }
}

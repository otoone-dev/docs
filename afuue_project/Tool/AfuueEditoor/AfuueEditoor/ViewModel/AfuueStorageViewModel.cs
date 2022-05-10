using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace AfuueEditoor.ViewModel
{
    public class AfuueStorageSlotViewModel
    {
        public string SlotName { get; set; } = "SlotName1";
        public bool IsSelected { get; set; } = false;
        public AfuueStorageSlotViewModel(string slotName)
        {
            SlotName = slotName;
        }
    }

    public class AfuueStorageViewModel
    {
        public ObservableCollection<AfuueStorageSlotViewModel> AfuueStorageItems { get; set; }
        public AfuueStorageViewModel()
        {
            AfuueStorageItems = new ObservableCollection<AfuueStorageSlotViewModel>();
            AfuueStorageItems.Add(new AfuueStorageSlotViewModel("Slot-1"));
            AfuueStorageItems.Add(new AfuueStorageSlotViewModel("Slot-2"));
            AfuueStorageItems.Add(new AfuueStorageSlotViewModel("Slot-3"));
            AfuueStorageItems.Add(new AfuueStorageSlotViewModel("Slot-4"));
            AfuueStorageItems.Add(new AfuueStorageSlotViewModel("Slot-5"));
            AfuueStorageItems[0].IsSelected = true;
        }
    }
}

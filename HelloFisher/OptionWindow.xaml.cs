using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace HelloFisher
{
  /// <summary>
  /// Interaction logic for OptionWindow.xaml
  /// </summary>
  public partial class OptionWindow : Window
  {
    OptionModel m_model;

    public OptionWindow()
    {
      InitializeComponent();
    }

    private void Window_Loaded(object sender, RoutedEventArgs e)
    {
      m_model = DataContext as OptionModel;
      m_model.FoodHotkey = FishingBoat.GetInteger("FoodHotkey");
      m_model.FoodTime = FishingBoat.GetInteger("FoodTime");
      m_model.FoodEnabled = FishingBoat.GetBoolean("FoodEnabled");
    }

    private void Window_Closed(object sender, EventArgs e)
    {
    }
  }

  class OptionModel : INotifyPropertyChanged
  {
    public event PropertyChangedEventHandler PropertyChanged;

    protected bool SetField<T>(ref T field, T value, [CallerMemberName] string propertyName = null)
    {
      if (!EqualityComparer<T>.Default.Equals(field, value))
      {
        field = value;
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        return true;
      }

      return false;
    }

    int _foodHotkey;
    public int FoodHotkey
    {
      get => _foodHotkey;
      set
      {
        if (SetField(ref _foodHotkey, value))
        {
          FishingBoat.SetInteger("FoodHotkey", value);
        }
      }
    }

    int _foodTime;
    public int FoodTime
    {
      get => _foodTime;
      set
      {
        if (SetField(ref _foodTime, value))
        {
          FishingBoat.SetInteger("FoodTime", value);
        }
      }
    }

    bool _foodEnabled;
    public bool FoodEnabled
    {
      get => _foodEnabled;
      set
      {
        if (SetField(ref _foodEnabled, value))
        {
          FishingBoat.SetBoolean("FoodEnabled", value);
        }
      }
    }
  }
}

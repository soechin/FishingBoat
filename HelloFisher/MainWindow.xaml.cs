using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading;
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

namespace HelloFisher
{
  /// <summary>
  /// Interaction logic for MainWindow.xaml
  /// </summary>
  public partial class MainWindow : Window
  {
    MainModel m_model;
    FishingBoat.LogFunc m_logFunc;
    volatile bool m_running;
    Thread m_thread1;
    Thread m_thread2;
    FishingSteps m_step;
    bool m_enabled;
    bool m_semiauto;

    public MainWindow()
    {
      InitializeComponent();
    }

    private void Window_Loaded(object sender, RoutedEventArgs e)
    {
      m_model = DataContext as MainModel;

      m_logFunc = new FishingBoat.LogFunc(LogFunc);
      FishingBoat.LogCallback(m_logFunc);

      m_running = true;
      m_thread1 = new Thread(Thread_Func1);
      m_thread2 = new Thread(Thread_Func2);
      m_thread1.Start();
      m_thread2.Start();

      m_model.DropGold = FishingBoat.GetBoolean("DropGold");
      m_model.DropBlue = FishingBoat.GetBoolean("DropBlue");
      m_model.DropGreen = FishingBoat.GetBoolean("DropGreen");
      m_model.Templates = FishingBoat.GetText("Templates");
    }

    private void Window_Closed(object sender, EventArgs e)
    {
      m_running = false;
      m_thread1.Join();
      m_thread2.Join();
    }

    private void Thread_Func1()
    {
      while (m_running)
      {
        if ((User32.GetAsyncKeyState(0x91/*SCROLL*/) & 0x0001) != 0)
        {
          m_enabled = (User32.GetKeyState(0x91/*SCROLL*/) & 0x0001) != 0;
          if (m_enabled) m_semiauto = true;
        }

        if ((User32.GetAsyncKeyState(0xC0/*~*/) & 0x0001) != 0)
        {
          m_semiauto = true;
        }

        Thread.Sleep(100);
      }
    }

    private void Thread_Func2()
    {
      while (m_running)
      {
        if (m_enabled || m_semiauto)
        {
          m_step = RunOnce();

          if (m_step == FishingSteps.IDLE)
          {
            m_enabled = false;
            m_semiauto = false;
          }
        }
        else
        {
          m_step = FishingSteps.START;
          Thread.Sleep(100);
        }

        if (m_semiauto)
        {
          if (m_step == FishingSteps.START_LOOP)
          {
            m_step = FishingSteps.STOP;
          }
          else if (m_step == FishingSteps.STOP_LOOP)
          {
            m_step = FishingSteps.STOP;
            m_semiauto = false;
          }
          else if (m_step == FishingSteps.RESTART)
          {
            m_semiauto = false;
          }
        }

        m_model.Enabled = m_enabled;
        m_model.Semiauto = m_semiauto;
        m_model.Status = StepToText(m_step);
      }
    }

    private FishingSteps RunOnce()
    {
      switch (m_step)
      {
        case FishingSteps.START:
          return FishingBoat.StartFishing();
        case FishingSteps.START_LOOP:
          return FishingBoat.StartLoop();
        case FishingSteps.STOP:
          return FishingBoat.StopFishing();
        case FishingSteps.STOP_LOOP:
          return FishingBoat.StopLoop();
        case FishingSteps.GUESS_WASD:
          return FishingBoat.GuessWasd();
        case FishingSteps.INPUT_TEXT:
          return FishingBoat.InputText();
        case FishingSteps.TAKE_DROP:
          return FishingBoat.TakeDrop();
        case FishingSteps.RESTART:
          return FishingBoat.RestartFishing();
      }

      return FishingSteps.IDLE;
    }

    private string StepToText(FishingSteps step)
    {
      switch (step)
      {
        case FishingSteps.START: return "準備";
        case FishingSteps.START_LOOP: return "等待開始...";
        case FishingSteps.STOP: return "收竿";
        case FishingSteps.STOP_LOOP: return "等待重試...";
        case FishingSteps.GUESS_WASD: return "文字辨識";
        case FishingSteps.INPUT_TEXT: return FishingBoat.GetText("WASD");
        case FishingSteps.TAKE_DROP: return "撿取物品";
        case FishingSteps.RESTART: return "拋竿";
      }

      return null;
    }

    private void LogFunc(string str)
    {
      Console.WriteLine(str);
    }

    private void DropGold_Click(object sender, RoutedEventArgs e)
    {
      FishingBoat.SetBoolean("DropGold", m_model.DropGold);
    }

    private void DropBlue_Click(object sender, RoutedEventArgs e)
    {
      FishingBoat.SetBoolean("DropBlue", m_model.DropBlue);
    }

    private void DropGreen_Click(object sender, RoutedEventArgs e)
    {
      FishingBoat.SetBoolean("DropGreen", m_model.DropGreen);
    }
  }

  class MainModel : INotifyPropertyChanged
  {
    public event PropertyChangedEventHandler PropertyChanged;

    protected void SetField<T>(ref T field, T value, [CallerMemberName] string propertyName = null)
    {
      if (!EqualityComparer<T>.Default.Equals(field, value))
      {
        field = value;
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
      }
    }

    bool _enabled = false;
    public bool Enabled
    {
      get => _enabled;
      set => SetField(ref _enabled, value);
    }

    bool _semiauto = false;
    public bool Semiauto
    {
      get => _semiauto;
      set => SetField(ref _semiauto, value);
    }

    string _status;
    public string Status
    {
      get => _status;
      set => SetField(ref _status, value);
    }

    bool _dropGold;
    public bool DropGold
    {
      get => _dropGold;
      set => SetField(ref _dropGold, value);
    }

    bool _dropBlue;
    public bool DropBlue
    {
      get => _dropBlue;
      set => SetField(ref _dropBlue, value);
    }

    bool _dropGreen;
    public bool DropGreen
    {
      get => _dropGreen;
      set => SetField(ref _dropGreen, value);
    }

    string _templates;
    public string Templates
    {
      get => _templates;
      set => SetField(ref _templates, value);
    }
  }
}

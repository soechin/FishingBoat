using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
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
    OptionWindow m_optWnd;

    public MainWindow()
    {
      InitializeComponent();
    }

    private void Window_Loaded(object sender, RoutedEventArgs e)
    {
      m_model = DataContext as MainModel;
      m_model.DropGold = FishingBoat.GetBoolean("DropGold");
      m_model.DropBlue = FishingBoat.GetBoolean("DropBlue");
      m_model.DropGreen = FishingBoat.GetBoolean("DropGreen");
      m_model.Templates = FishingBoat.GetText("Templates");

      m_logFunc = new FishingBoat.LogFunc(LogFunc);
      FishingBoat.LogCallback(m_logFunc);

      m_running = true;
      m_thread1 = new Thread(Thread_Func1);
      m_thread2 = new Thread(Thread_Func2);
      m_thread1.Start();
      m_thread2.Start();
    }

    private void Window_Closed(object sender, EventArgs e)
    {
      m_running = false;
      m_thread1.Join();
      m_thread2.Join();

      if (m_optWnd != null)
      {
        m_optWnd.Close();
      }
    }

    private void Window_LocationChanged(object sender, EventArgs e)
    {
      if (m_optWnd != null)
      {
        m_optWnd.Left = Left + Width;
        m_optWnd.Top = Top;
      }
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

        if ((User32.GetAsyncKeyState(0x70/*F1*/) & 0x0001) != 0)
        {
          m_semiauto = true;
        }

        Thread.Sleep(100);
      }
    }

    private void Thread_Func2()
    {
      Stopwatch watch = Stopwatch.StartNew();

      while (m_running)
      {
        if (m_enabled || m_semiauto)
        {
          if (m_step == FishingSteps.START)
          {
            watch.Restart();
          }
          else if (m_step == FishingSteps.RESTART)
          {
            m_semiauto = false;
          }

          try
          {
            m_step = RunOnce();
          }
          catch (SEHException)
          {
            m_step = FishingSteps.IDLE;
          }

          if (m_step == FishingSteps.IDLE || m_step == FishingSteps.RESTART)
          {
            m_model.Timer = string.Format("{0:F1} 秒", watch.Elapsed.TotalSeconds);

            if (m_step == FishingSteps.IDLE)
            {
              m_enabled = false;
              m_semiauto = false;
            }
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
        }

        m_model.Enabled = m_enabled;
        m_model.Semiauto = m_semiauto;
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
      Dispatcher.BeginInvoke(new Action(() =>
      {
        m_model.Logs.Insert(0, str);
      }));
    }

    private void OptionButton_Click(object sender, RoutedEventArgs e)
    {
      if (m_optWnd != null)
      {
        m_optWnd.Close();
        m_optWnd = null;
        return;
      }

      m_optWnd = new OptionWindow
      {
        Owner = this
      };

      m_optWnd.Left = Left + Width;
      m_optWnd.Top = Top;
      m_optWnd.Show();
    }

    private void LogButton_Click(object sender, RoutedEventArgs e)
    {
      if (m_model.LogVisible == Visibility.Visible)
      {
        m_model.LogVisible = Visibility.Collapsed;
      }
      else
      {
        m_model.LogVisible = Visibility.Visible;
      }
    }
  }

  class MainModel : INotifyPropertyChanged
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

    string _timer;
    public string Timer
    {
      get => _timer;
      set => SetField(ref _timer, value);
    }

    ObservableCollection<string> _logs = new ObservableCollection<string>();
    public ObservableCollection<string> Logs
    {
      get => _logs;
      set => SetField(ref _logs, value);
    }

    bool _dropGold;
    public bool DropGold
    {
      get => _dropGold;
      set
      {
        if (SetField(ref _dropGold, value))
        {
          FishingBoat.SetBoolean("DropGold", value);
        }
      }
    }

    bool _dropBlue;
    public bool DropBlue
    {
      get => _dropBlue;
      set
      {
        if (SetField(ref _dropBlue, value))
        {
          FishingBoat.SetBoolean("DropBlue", value);
        }
      }
    }

    bool _dropGreen;
    public bool DropGreen
    {
      get => _dropGreen;
      set
      {
        if (SetField(ref _dropGreen, value))
        {
          FishingBoat.SetBoolean("DropGreen", value);
        }
      }
    }

    string _templates;
    public string Templates
    {
      get => _templates;
      set => SetField(ref _templates, value);
    }

    Visibility _logVisible = Visibility.Collapsed;
    public Visibility LogVisible
    {
      get => _logVisible;
      set => SetField(ref _logVisible, value);
    }
  }
}

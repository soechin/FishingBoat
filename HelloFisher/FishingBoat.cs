using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace HelloFisher
{
  public class FishingBoat
  {
    [DllImport("FishingBoat.dll")]
    public static extern FishingSteps StartFishing();

    [DllImport("FishingBoat.dll")]
    public static extern FishingSteps StartLoop();

    [DllImport("FishingBoat.dll")]
    public static extern FishingSteps StopFishing();

    [DllImport("FishingBoat.dll")]
    public static extern FishingSteps StopLoop();

    [DllImport("FishingBoat.dll")]
    public static extern FishingSteps GuessWasd();

    [DllImport("FishingBoat.dll")]
    public static extern FishingSteps InputText();

    [DllImport("FishingBoat.dll")]
    public static extern FishingSteps TakeDrop();

    [DllImport("FishingBoat.dll")]
    public static extern FishingSteps RestartFishing();

    [DllImport("FishingBoat.dll")]
    public static extern bool GetBoolean(string key);

    [DllImport("FishingBoat.dll")]
    public static extern void SetBoolean(string key, bool val);

    [DllImport("FishingBoat.dll")]
    public static extern int GetInteger(string key);

    [DllImport("FishingBoat.dll")]
    public static extern void SetInteger(string key, int val);

    [DllImport("FishingBoat.dll")]
    public static extern void GetRect(string key, out int x, out int y, out int width, out int height);

    [DllImport("FishingBoat.dll")]
    public static extern void SetRect(string key, int x, int y, int width, int height);

    [DllImport("FishingBoat.dll")]
    [return: MarshalAs(UnmanagedType.BStr)]
    public static extern string GetText(string key);

    [DllImport("FishingBoat.dll")]
    [return: MarshalAs(UnmanagedType.BStr)]
    public static extern string GetImage(string key);

    public delegate void LogFunc([MarshalAs(UnmanagedType.LPWStr)] string str);
    [DllImport("FishingBoat.dll")]
    public static extern void LogCallback(LogFunc func);
  }

  public enum FishingSteps
  {
    IDLE = 0,
    START = 1,
    START_LOOP = 2,
    STOP = 3,
    STOP_LOOP = 4,
    GUESS_WASD = 5,
    INPUT_TEXT = 6,
    TAKE_DROP = 7,
    RESTART = 8,
  }
}

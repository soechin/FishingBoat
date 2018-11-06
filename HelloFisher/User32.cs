using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace HelloFisher
{
  class User32
  {
    [DllImport("User32.dll")]
    public static extern short GetAsyncKeyState(int vk);

    [DllImport("User32.dll")]
    public static extern short GetKeyState(int vk);
  }
}

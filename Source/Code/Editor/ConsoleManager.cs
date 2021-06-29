using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace Editor
{
    class ConsoleManager
    {
        [DllImport(@"kernel32.dll", SetLastError = true)]
        static extern bool AllocConsole();

        [DllImport(@"kernel32.dll", SetLastError = true)]
        static extern bool FreeConsole();

        public static void ShowConsole()
        {
            AllocConsole();
        }

        public static void HideConsole()
        {
            FreeConsole();
        }
    }
}

using System;
using System.Runtime.InteropServices;

namespace XAPI
{
    [ComVisible(false)]
    public class SoInvoke : InvokeBase
    {
        [DllImport("libdl.so")]
        private extern static IntPtr dlopen(string path, int mode);
        [DllImport("libdl.so")]
        private extern static IntPtr dlsym(IntPtr handle, string symbol);
        [DllImport("libdl.so")]
        private extern static IntPtr dlerror();
        [DllImport("libdl.so")]
        private extern static int dlclose(IntPtr handle);

        public SoInvoke(string DLLPath)
        {
            hLib = dlopen(DLLPath, 1);
            if (hLib == IntPtr.Zero)
            {
                IntPtr errptr = dlerror();
                string error = Marshal.PtrToStringAnsi(errptr);
                throw new Exception(error);
            }
        }
        ~SoInvoke()
        {
            Dispose();
        }
        //将要执行的函数转换为委托
        public override T Invoke<T>(String APIName)
        {
            if (hLib == IntPtr.Zero)
                return default(T);

            IntPtr api = dlsym(hLib, APIName);
            return Marshal.GetDelegateForFunctionPointer<T>(api);
        }

        public override void Dispose()
        {
            if (hLib != IntPtr.Zero)
                dlclose(hLib);
            hLib = IntPtr.Zero;
        }
    }
}

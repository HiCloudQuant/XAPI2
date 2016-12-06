using System;
using System.Runtime.InteropServices;

namespace XAPI
{
    [ComVisible(false)]
    public class InvokeBase
    {
        protected IntPtr hLib = IntPtr.Zero;
        public virtual T Invoke<T>(String APIName)
        {
            return default(T);
        }

        public virtual void Dispose()
        {

        }
    }
}

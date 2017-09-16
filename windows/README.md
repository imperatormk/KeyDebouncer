Simply compile into an .exe, and then run. Visual Studio and/or g++ through Cygwin/LXSS is good for this.

Note: Currently requires some c++11 features. Compilation may require a "-std=c++11" flag.

Optionally accepts a single command line argument that specifies the time window to use for debouncing in milliseconds. 
Going lower than around 15ms is not recommended because of concerns raised in articles like this
http://web.archive.org/web/20121018065403/http://msdn.microsoft.com/en-us/magazine/cc163996.aspx

---

Please feel free to contribute to fix any problems!

Please note, this repo is for educational purposes only. No contributors are to fault for any actions done by this program.

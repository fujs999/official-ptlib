//
// hello.cxx
//
// Equivalence Pty. Ltd.
//

#include <ptlib.h>
#include <ptlib/pprocess.h>

class Hello : public PProcess
{
  PCLASSINFO(Hello, PProcess)
  public:
    void Main();
};

PCREATE_PROCESS(Hello)

void Hello::Main()
{
  MemoryUsage usage;
  GetMemoryUsage(usage);
  cout << "Hello world!\n\n"
          "From " << GetOSClass() << ' ' << GetOSName() << " (" << GetOSVersion() << ")"
          " on " << GetOSHardware() << ", PTLib version " << GetLibVersion() << "\n\n"
          "Memory total    : " << PString(PString::ScaleSI, usage.m_total,     4) << "b\n"
          "Memory available: " << PString(PString::ScaleSI, usage.m_available, 4) << "b\n"
          "Virtual used    : " << PString(PString::ScaleSI, usage.m_virtual,   4) << "b\n"
          "Resident used   : " << PString(PString::ScaleSI, usage.m_resident,  4) << "b\n"
       << endl;
}

// End of hello.cxx

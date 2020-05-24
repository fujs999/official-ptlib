
#include <ptlib.h>
#include <ptlib/video.h>
#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>

class VideoConsumer;
class BMediaRoster;
class PVideoInputThread;
#include <MediaNode.h>

/**This class defines a BeOS video input device.
 */
class PVideoInputDevice_BeOSVideo : public PVideoInputDevice
{
  PCLASSINFO(PVideoInputDevice_BeOSVideo, PVideoInputDevice);

  public:
    /** Create a new video input device.
     */
    PVideoInputDevice_BeOSVideo();

    /**Close the video input device on destruction.
      */
    ~PVideoInputDevice_BeOSVideo() { Close(); }

    /** Is the device a camera, and obtain video
     */
    static PStringList GetInputDeviceNames();

    virtual PStringList GetDeviceNames() const
      { return GetInputDeviceNames(); }

    /**Open the device given the device name.
      */
    virtual bool Open(
      const PString & deviceName,   /// Device name to open
      bool startImmediate = true    /// Immediately start device
    );

    /**Determine if the device is currently open.
      */
    virtual bool IsOpen();

    /**Close the device.
      */
    virtual bool Close();

    /**Start the video device I/O.
      */
    virtual bool Start();

    /**Stop the video device I/O capture.
      */
    virtual bool Stop();

    /**Determine if the video device I/O capture is in progress.
      */
    virtual bool IsCapturing();

    /**Get the maximum frame size in bytes.

       Note a particular device may be able to provide variable length
       frames (eg motion JPEG) so will be the maximum size of all frames.
      */
    virtual PINDEX GetMaxFrameBytes();

    /**Grab a frame.
      */
    virtual bool GetFrame(
      PBYTEArray & frame
    );

    /**Try all known video formats & see which ones are accepted by the video driver
     */
    virtual bool TestAllFormats();

  public:
    virtual bool SetColourFormat(const PString & colourFormat);
    virtual bool SetFrameRate(unsigned rate);
    virtual bool SetFrameSize(unsigned width, unsigned height);

    friend PVideoInputThread;
  private:
    status_t StartNodes();
    void StopNodes();

  protected:
    virtual bool InternalGetFrameData(uint8_t * buffer, PINDEX & bytesReturned, bool & keyFrame, bool wait);

    BMediaRoster* fMediaRoster;
    VideoConsumer* fVideoConsumer;

    media_output fProducerOut;
    media_input fConsumerIn;

    media_node fTimeSourceNode;
    media_node fProducerNode;

    port_id fPort;

    bool          isCapturingNow;
    PVideoInputThread* captureThread;
    
};


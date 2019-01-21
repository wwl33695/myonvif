#include "H264VideoStreamFramer.hh"


//*********************************************************************
// ��ȡ����ͷ��ƵYUV���ݺ󣬱����H264�������͸�DSS
class H264EncWrapper;
struct TNAL;

class CamH264VideoStreamFramer : public H264VideoStreamFramer
{
public:
	virtual ~CamH264VideoStreamFramer();
	CamH264VideoStreamFramer(UsageEnvironment& env,
		FramedSource* inputSource, H264EncWrapper* pH264Enc);

	static CamH264VideoStreamFramer* createNew(UsageEnvironment& env, FramedSource* inputSource);
	virtual Boolean currentNALUnitEndsAccessUnit();
	virtual void doGetNextFrame();

private:
	H264EncWrapper* m_pH264Enc;

	TNAL* m_pNalArray;
	int m_iCurNalNum;		//��ǰFrameһ���ж��ٸ�NAL
	int m_iCurNal;		//��ǰʹ�õ��ǵڼ���NAL
	unsigned int m_iCurFrame;
};

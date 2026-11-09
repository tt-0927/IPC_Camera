/**
 * @FilePath     : classroom_osd.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-20 17:20:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-20 17:20:00
 * @Description  : 课堂框送整机 OSD 实现
 */

#include "classroom_osd.h"

#include "common_process.h"    /* send_detectionResult_to_osd 与 Common::RectInfo_S 都来自这个重头 */

namespace Classroom
{
void classroomOsdPush(const CFrameGeometry &stGeom, const std::vector<CRectf_S> &vecCanvasBox)
{
    if (!stGeom.isReady())
    {
        /* 几何没建起来就不送：此时分析帧坐标本身就是无定义的东西，
           画出一个乱框比什么都不画更容易把人引到错方向 */
        return;
    }

    /* 复用一张表而不是每次局部构造：本函数每拍一次，局部 vector 会来回 new/free。
       只由节拍线程调用（见头文件线程模型），故不加锁；哪天有第二个调用线程，
       要么改回局部表，要么在此加锁——两者都比现在多一次开销，但不要留着不改 */
    static std::vector<Common::RectInfo_S> s_vecRect;
    s_vecRect.clear();

    const int nSrcW = stGeom.srcWidth();
    const int nSrcH = stGeom.srcHeight();

    for (size_t i = 0; i < vecCanvasBox.size(); ++i)
    {
        const CRectf_S stSrc = stGeom.dstToSrc(vecCanvasBox[i]);

        Common::RectInfo_S stRect;
        stRect.nX1 = static_cast<int>(clampValue(stSrc.fX1, 0.0f, static_cast<float>(nSrcW - 1)));
        stRect.nY1 = static_cast<int>(clampValue(stSrc.fY1, 0.0f, static_cast<float>(nSrcH - 1)));
        stRect.nX2 = static_cast<int>(clampValue(stSrc.fX2, 0.0f, static_cast<float>(nSrcW - 1)));
        stRect.nY2 = static_cast<int>(clampValue(stSrc.fY2, 0.0f, static_cast<float>(nSrcH - 1)));

        if (stRect.nX2 <= stRect.nX1 || stRect.nY2 <= stRect.nY1)
        {
            /* 换算后退化成一个点：贴边的框十有八九是这个下场 */
            continue;
        }
        s_vecRect.push_back(stRect);
    }

    send_detectionResult_to_osd(nSrcW, nSrcH, s_vecRect);
}
}

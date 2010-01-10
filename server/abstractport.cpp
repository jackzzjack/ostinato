#include "abstractport.h"

#include <QString>
#include <QIODevice>

#include "../common/streambase.h"

AbstractPort::AbstractPort(int id, const char *device)
{
    data_.mutable_port_id()->set_id(id);
    data_.set_name(device);

    //! \todo (LOW) admin enable/disable of port
    data_.set_is_enabled(true);

    //! \todo (HIGH) port exclusive control
    data_.set_is_exclusive_control(false);

    isSendQueueDirty_ = true;
    linkState_ = OstProto::LinkStateUnknown;

    memset((void*) &stats_, 0, sizeof(stats_));
    resetStats();
}

void AbstractPort::init()
{
}    

AbstractPort::~AbstractPort()
{
}    

StreamBase* AbstractPort::streamAtIndex(int index)
{
    Q_ASSERT(index < streamList_.size());
    return streamList_.at(index);
}

StreamBase* AbstractPort::stream(int streamId)
{
    for (int i = 0; i < streamList_.size(); i++)
    {
        if (streamId == streamList_.at(i)->id())
            return streamList_.at(i);
    }

    return NULL;
}

bool AbstractPort::addStream(StreamBase *stream)
{
    streamList_.append(stream);
    isSendQueueDirty_ = true;
    return true;
}

bool AbstractPort::deleteStream(int streamId)
{
    for (int i = 0; i < streamList_.size(); i++)
    {
        StreamBase *stream;

        if (streamId == streamList_.at(i)->id())
        {
            stream = streamList_.takeAt(i);
            delete stream;
            
            isSendQueueDirty_ = true;
            return true;
        }
    }

    return false;
}

void AbstractPort::updatePacketList()
{
    int     len;
    bool    isVariable;
    uchar   pktBuf[2000];
    long    sec = 0; 
    long    usec = 0;

    qDebug("In %s", __FUNCTION__);

    // First sort the streams by ordinalValue
    qSort(streamList_.begin(), streamList_.end(), StreamBase::StreamLessThan);

    clearPacketList();

    for (int i = 0; i < streamList_.size(); i++)
    {
        if (streamList_[i]->isEnabled())
        {
            long numPackets, numBursts;
            long ibg, ipg;

            switch (streamList_[i]->sendUnit())
            {
            case OstProto::StreamControl::e_su_bursts:
                numBursts = streamList_[i]->numBursts();
                numPackets = streamList_[i]->burstSize();
                ibg = 1000000/streamList_[i]->burstRate();
                ipg = 0;
                break;
            case OstProto::StreamControl::e_su_packets:
                numBursts = 1;
                numPackets = streamList_[i]->numPackets();
                ibg = 0;
                ipg = 1000000/streamList_[i]->packetRate();
                break;
            default:
                qWarning("Unhandled stream control unit %d",
                    streamList_[i]->sendUnit());
                continue;
            }
            qDebug("numBursts = %ld, numPackets = %ld\n",
                    numBursts, numPackets);
            qDebug("ibg = %ld, ipg = %ld\n", ibg, ipg);

            if (streamList_[i]->isFrameVariable())
            {
                isVariable = true;
            }
            else
            {
                isVariable = false;
                len = streamList_[i]->frameValue(pktBuf, sizeof(pktBuf), 0);
            }

            for (int j = 0; j < numBursts; j++)
            {
                for (int k = 0; k < numPackets; k++)
                {
                    if (isVariable)
                    {
                        len = streamList_[i]->frameValue(pktBuf, 
                                sizeof(pktBuf), j * numPackets + k);
                    }
                    if (len <= 0)
                        continue;

                    qDebug("q(%d, %d, %d) sec = %lu usec = %lu",
                            i, j, k, sec, usec);

                    appendToPacketList(sec, usec, pktBuf, len); 

                    usec += ipg;
                    if (usec > 1000000)
                    {
                        sec++;
                        usec -= 1000000;
                    }
                } // for (numPackets)

                usec += ibg;
                if (usec > 1000000)
                {
                    sec++;
                    usec -= 1000000;
                }
            } // for (numBursts)

            switch(streamList_[i]->nextWhat())
            {
                case ::OstProto::StreamControl::e_nw_stop:
                    goto _stop_no_more_pkts;

                case ::OstProto::StreamControl::e_nw_goto_id:
                    /*! \todo (MED): define and use 
                    streamList_[i].d.control().goto_stream_id(); */

                    /*! \todo (MED): assumes goto Id is less than current!!!! 
                     To support goto to any id, do
                     if goto_id > curr_id then 
                         i = goto_id;
                         goto restart;
                     else
                         returnToQIdx = 0;
                     */

                    setPacketListLoopMode(true, streamList_[i]->sendUnit() == 
                          OstProto::StreamControl::e_su_bursts ? ibg : ipg);
                    goto _stop_no_more_pkts;

                case ::OstProto::StreamControl::e_nw_goto_next:
                    break;

                default:
                    qFatal("---------- %s: Unhandled case (%d) -----------",
                            __FUNCTION__, streamList_[i]->nextWhat() );
                    break;
            }

        } // if (stream is enabled)
    } // for (numStreams)

_stop_no_more_pkts:
    isSendQueueDirty_ = false;
}

void AbstractPort::stats(PortStats *stats)
{
    stats->rxPkts = stats_.rxPkts - epochStats_.rxPkts; 
    stats->rxBytes = stats_.rxBytes - epochStats_.rxBytes; 
    stats->rxPps = stats_.rxPps; 
    stats->rxBps = stats_.rxBps; 

    stats->txPkts = stats_.txPkts - epochStats_.txPkts; 
    stats->txBytes = stats_.txBytes - epochStats_.txBytes; 
    stats->txPps = stats_.txPps; 
    stats->txBps = stats_.txBps; 
}

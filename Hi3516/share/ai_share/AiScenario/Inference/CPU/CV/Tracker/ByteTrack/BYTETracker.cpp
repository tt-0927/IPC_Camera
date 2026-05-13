
/*
 * @FilePath     : BYTETracker.cpp
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-09-23 19:48:15
 * @LastEditors  : 吴才朋 wucp@kfb.cn
 * @LastEditTime : 2024-09-23 19:48:15
 * @Description  : Bytetrack跟踪算法接口
 */


#include "BYTETracker.h"
#include <fstream>

cBYTETracker::cBYTETracker(int nFrameRate, int nTrackBuffer)
{
	m_fTrackThresh = 0.5;
	m_fHighThresh = 0.6;
	m_fMatchThresh = 0.8;

	m_nFrameId = 0;
	m_nMaxTimeLost = int(nFrameRate / 30.0 * nTrackBuffer);
}

cBYTETracker::~cBYTETracker()
{
}

 std::vector<cSTrack> cBYTETracker::update(const  std::vector<DetectResult_S>& vObjects)
{

	////////////////// Step 1: Get detections //////////////////
	this->m_nFrameId++;
	 std::vector<cSTrack> activated_stracks;
	 std::vector<cSTrack> refind_stracks;
	 std::vector<cSTrack> removed_stracks;
	 std::vector<cSTrack> lost_stracks;
	 std::vector<cSTrack> detections;
	 std::vector<cSTrack> detections_low;

	 std::vector<cSTrack> detections_cp;
	 std::vector<cSTrack> tracked_stracks_swap;
	 std::vector<cSTrack> resa, resb;
	 std::vector<cSTrack> output_stracks;

	 std::vector<cSTrack*> unconfirmed;
	 std::vector<cSTrack*> tracked_stracks;
	 std::vector<cSTrack*> strack_pool;
	 std::vector<cSTrack*> r_tracked_stracks;

	if (vObjects.size() > 0)
	{
		for (int i = 0; i < vObjects.size(); i++)
		{
			 std::vector<float> tlbr_;
			tlbr_.resize(4);
            tlbr_[0] = vObjects[i].vfBox.x;
            tlbr_[1] = vObjects[i].vfBox.y;
            tlbr_[2] = vObjects[i].vfBox.x + vObjects[i].vfBox.width;
            tlbr_[3] = vObjects[i].vfBox.y + vObjects[i].vfBox.height;

            float score = vObjects[i].fConfidence;

			cSTrack strack(cSTrack::tlbr_to_tlwh(tlbr_), score);
			if (score >= m_fTrackThresh)
			{
				detections.push_back(strack);
			}
			else
			{
				detections_low.push_back(strack);
			}
			
		}
	}

	// Add newly detected tracklets to tracked_stracks
	for (int i = 0; i < this->tracked_stracks.size(); i++)
	{
		if (!this->tracked_stracks[i].is_activated)
			unconfirmed.push_back(&this->tracked_stracks[i]);
		else
			tracked_stracks.push_back(&this->tracked_stracks[i]);
	}

	////////////////// Step 2: First association, with IoU //////////////////
	strack_pool = joint_stracks(tracked_stracks, this->lost_stracks);
	cSTrack::multi_predict(strack_pool, this->kalman_filter);

	 std::vector< std::vector<float> > dists;
	int dist_size = 0, dist_size_size = 0;
	dists = iou_distance(strack_pool, detections, dist_size, dist_size_size);

	 std::vector< std::vector<int> > matches;
	 std::vector<int> u_track, u_detection;
	linear_assignment(dists, dist_size, dist_size_size, m_fMatchThresh, matches, u_track, u_detection);

	for (int i = 0; i < matches.size(); i++)
	{
		cSTrack *track = strack_pool[matches[i][0]];
		cSTrack *det = &detections[matches[i][1]];
		if (track->state == TrackState::Tracked)
		{
			track->update(*det, this->m_nFrameId);
			activated_stracks.push_back(*track);
		}
		else
		{
			track->re_activate(*det, this->m_nFrameId, false);
			refind_stracks.push_back(*track);
		}
	}

	////////////////// Step 3: Second association, using low score dets //////////////////
	for (int i = 0; i < u_detection.size(); i++)
	{
		detections_cp.push_back(detections[u_detection[i]]);
	}
	detections.clear();
	detections.assign(detections_low.begin(), detections_low.end());
	
	for (int i = 0; i < u_track.size(); i++)
	{
		if (strack_pool[u_track[i]]->state == TrackState::Tracked)
		{
			r_tracked_stracks.push_back(strack_pool[u_track[i]]);
		}
	}

	dists.clear();
	dists = iou_distance(r_tracked_stracks, detections, dist_size, dist_size_size);

	matches.clear();
	u_track.clear();
	u_detection.clear();
	linear_assignment(dists, dist_size, dist_size_size, 0.5, matches, u_track, u_detection);

	for (int i = 0; i < matches.size(); i++)
	{
		cSTrack *track = r_tracked_stracks[matches[i][0]];
		cSTrack *det = &detections[matches[i][1]];
		if (track->state == TrackState::Tracked)
		{
			track->update(*det, this->m_nFrameId);
			activated_stracks.push_back(*track);
		}
		else
		{
			track->re_activate(*det, this->m_nFrameId, false);
			refind_stracks.push_back(*track);
		}
	}

	for (int i = 0; i < u_track.size(); i++)
	{
		cSTrack *track = r_tracked_stracks[u_track[i]];
		if (track->state != TrackState::Lost)
		{
			track->mark_lost();
			lost_stracks.push_back(*track);
		}
	}

	// Deal with unconfirmed tracks, usually tracks with only one beginning frame
	detections.clear();
	detections.assign(detections_cp.begin(), detections_cp.end());

	dists.clear();
	dists = iou_distance(unconfirmed, detections, dist_size, dist_size_size);

	matches.clear();
	 std::vector<int> u_unconfirmed;
	u_detection.clear();
	linear_assignment(dists, dist_size, dist_size_size, 0.7, matches, u_unconfirmed, u_detection);

	for (int i = 0; i < matches.size(); i++)
	{
		unconfirmed[matches[i][0]]->update(detections[matches[i][1]], this->m_nFrameId);
		activated_stracks.push_back(*unconfirmed[matches[i][0]]);
	}

	for (int i = 0; i < u_unconfirmed.size(); i++)
	{
		cSTrack *track = unconfirmed[u_unconfirmed[i]];
		track->mark_removed();
		removed_stracks.push_back(*track);
	}

	////////////////// Step 4: Init new stracks //////////////////
	for (int i = 0; i < u_detection.size(); i++)
	{
		cSTrack *track = &detections[u_detection[i]];
		if (track->score < this->m_fHighThresh)
			continue;
		track->activate(this->kalman_filter, this->m_nFrameId);
		activated_stracks.push_back(*track);
	}

	////////////////// Step 5: Update state //////////////////
	for (int i = 0; i < this->lost_stracks.size(); i++)
	{
		if (this->m_nFrameId - this->lost_stracks[i].end_frame() > this->m_nMaxTimeLost)
		{
			this->lost_stracks[i].mark_removed();
			removed_stracks.push_back(this->lost_stracks[i]);
		}
	}
	
	for (int i = 0; i < this->tracked_stracks.size(); i++)
	{
		if (this->tracked_stracks[i].state == TrackState::Tracked)
		{
			tracked_stracks_swap.push_back(this->tracked_stracks[i]);
		}
	}
	this->tracked_stracks.clear();
	this->tracked_stracks.assign(tracked_stracks_swap.begin(), tracked_stracks_swap.end());

	this->tracked_stracks = joint_stracks(this->tracked_stracks, activated_stracks);
	this->tracked_stracks = joint_stracks(this->tracked_stracks, refind_stracks);

	//std::cout << activated_stracks.size() << std::endl;

	this->lost_stracks = sub_stracks(this->lost_stracks, this->tracked_stracks);
	for (int i = 0; i < lost_stracks.size(); i++)
	{
		this->lost_stracks.push_back(lost_stracks[i]);
	}

	this->lost_stracks = sub_stracks(this->lost_stracks, this->removed_stracks);
	for (int i = 0; i < removed_stracks.size(); i++)
	{
		this->removed_stracks.push_back(removed_stracks[i]);
	}
	
	remove_duplicate_stracks(resa, resb, this->tracked_stracks, this->lost_stracks);

	this->tracked_stracks.clear();
	this->tracked_stracks.assign(resa.begin(), resa.end());
	this->lost_stracks.clear();
	this->lost_stracks.assign(resb.begin(), resb.end());
	
	for (int i = 0; i < this->tracked_stracks.size(); i++)
	{
		if (this->tracked_stracks[i].is_activated)
		{
			output_stracks.push_back(this->tracked_stracks[i]);
		}
	}
	return output_stracks;
}

/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#include <vector>			// // //
#include "stdafx.h"
#include "FamiTrackerDoc.h"
#include "Instrument.h"
#include "Compiler.h"		// // //
#include "Chunk.h"
#include "DocumentFile.h"

const int CInstrumentS5B::SEQUENCE_TYPES[] = {SEQ_VOLUME, SEQ_ARPEGGIO, SEQ_PITCH, SEQ_HIPITCH, SEQ_DUTYCYCLE};

CInstrumentS5B::CInstrumentS5B()
{
	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		m_iSeqEnable[i] = 0;
		m_iSeqIndex[i] = 0;
	}
}

CInstrument *CInstrumentS5B::Clone() const
{
	CInstrumentS5B *pNew = new CInstrumentS5B();

	for (int i = 0; i < SEQUENCE_COUNT; i++) {
		pNew->SetSeqEnable(i, GetSeqEnable(i));
		pNew->SetSeqIndex(i, GetSeqIndex(i));
	}

	pNew->SetName(GetName());

	return pNew;
}

void CInstrumentS5B::Setup()
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();		// // //

	for (int i = 0; i < SEQ_COUNT; ++i) {
		SetSeqEnable(i, 0);
		int Index = pDoc->GetFreeSequenceS5B(i);		// // //
		if (Index != -1)
			SetSeqIndex(i, Index);
	}
}

void CInstrumentS5B::Store(CDocumentFile *pDocFile)
{
	pDocFile->WriteBlockInt(SEQUENCE_COUNT);

	for (int i = 0; i < SEQUENCE_COUNT; i++) {
		pDocFile->WriteBlockChar(GetSeqEnable(i));
		pDocFile->WriteBlockChar(GetSeqIndex(i));
	}
}

bool CInstrumentS5B::Load(CDocumentFile *pDocFile)
{
	int SeqCnt = pDocFile->GetBlockInt();

	ASSERT_FILE_DATA(SeqCnt < (SEQUENCE_COUNT + 1));

	SeqCnt = SEQUENCE_COUNT;

	for (int i = 0; i < SeqCnt; i++) {
		SetSeqEnable(i, pDocFile->GetBlockChar());
		int Index = pDocFile->GetBlockChar();
		ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
		SetSeqIndex(i, Index);
	}

	return true;
}

void CInstrumentS5B::SaveFile(CInstrumentFile *pFile, const CFamiTrackerDoc *pDoc)		// // //
{
	// Sequences
	pFile->WriteChar(SEQUENCE_COUNT);

	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		int Sequence = GetSeqIndex(i);
		if (GetSeqEnable(i)) {
			const CSequence *pSeq = pDoc->GetSequenceS5B(Sequence, i);

			pFile->WriteChar(1);
			pFile->WriteInt(pSeq->GetItemCount());
			pFile->WriteInt(pSeq->GetLoopPoint());
			pFile->WriteInt(pSeq->GetReleasePoint());
			pFile->WriteInt(pSeq->GetSetting());

			for (unsigned j = 0; j < pSeq->GetItemCount(); ++j) {
				pFile->WriteChar(pSeq->GetItem(j));
			}
		}
		else {
			pFile->WriteChar(0);
		}
	}
}

bool CInstrumentS5B::LoadFile(CInstrumentFile *pFile, int iVersion, CFamiTrackerDoc *pDoc)		// // //
{
	// Sequences
	stSequence OldSequence;

	unsigned char SeqCount = pFile->ReadChar();

	// Loop through all instrument effects
	for (int i = 0; i < SeqCount; ++i) {
		unsigned char Enabled = pFile->ReadChar();
		if (Enabled == 1) {
			// Read the sequence
			int Count = pFile->ReadInt();
			int Index = pDoc->GetFreeSequenceS5B(i);
			if (Index != -1) {
				CSequence *pSeq = pDoc->GetSequence(SNDCHIP_S5B, Index, i);

				if (iVersion < 20) {
					OldSequence.Count = Count;
					for (int j = 0; j < Count; ++j) {
						OldSequence.Length[j] = pFile->ReadChar();
						OldSequence.Value[j] = pFile->ReadChar();
					}
					CFamiTrackerDoc::ConvertSequence(&OldSequence, pSeq, i);	// convert
				}
				else {
					pSeq->SetItemCount(Count);
					pSeq->SetLoopPoint(pFile->ReadInt());
					if (iVersion > 20) {
						pSeq->SetReleasePoint(pFile->ReadInt());
					}
					if (iVersion >= 22) {
						pSeq->SetSetting(static_cast<seq_setting_t>(pFile->ReadInt()));		// // //
					}
					for (int j = 0; j < Count; ++j) {
						pSeq->SetItem(j, pFile->ReadChar());
					}
				}
				SetSeqEnable(i, true);
				SetSeqIndex(i, Index);
			}
		}
		else {
			SetSeqEnable(i, false);
			SetSeqIndex(i, 0);
		}
	}

	return true;
}

int CInstrumentS5B::Compile(CFamiTrackerDoc *pDoc, CChunk *pChunk, int Index)		// // //
{
	int ModSwitch = 0;
	int StoredBytes = 0;

	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		ModSwitch = (ModSwitch >> 1) | (GetSeqEnable(i) && (pDoc->GetSequence(SNDCHIP_S5B, GetSeqIndex(i), i)->GetItemCount() > 0) ? 0x10 : 0);
	}

	pChunk->StoreByte(ModSwitch);
	StoredBytes++;

	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		if (GetSeqEnable(i) != 0 && (pDoc->GetSequence(SNDCHIP_S5B, GetSeqIndex(i), i)->GetItemCount() != 0)) {
			CString str;
			str.Format(CCompiler::LABEL_SEQ_S5B, GetSeqIndex(i) * SEQUENCE_COUNT + i);
			pChunk->StoreReference(str);
			StoredBytes += 2;
		}
	}
	
	return StoredBytes;
}

bool CInstrumentS5B::CanRelease() const
{
	if (GetSeqEnable(0) != 0) {		// // //
		int index = GetSeqIndex(SEQ_VOLUME);
		return CFamiTrackerDoc::GetDoc()->GetSequence(SNDCHIP_S5B, index, SEQ_VOLUME)->GetReleasePoint() != -1;
	}

	return false;
}

int	CInstrumentS5B::GetSeqEnable(int Index) const
{
	return m_iSeqEnable[Index];
}

int	CInstrumentS5B::GetSeqIndex(int Index) const
{
	return m_iSeqIndex[Index];
}

void CInstrumentS5B::SetSeqEnable(int Index, int Value)
{
	if (m_iSeqEnable[Index] != Value)		// // //
		InstrumentChanged();		
	m_iSeqEnable[Index] = Value;
}

void CInstrumentS5B::SetSeqIndex(int Index, int Value)
{
	if (m_iSeqIndex[Index] != Value)		// // //
		InstrumentChanged();
	m_iSeqIndex[Index] = Value;
}
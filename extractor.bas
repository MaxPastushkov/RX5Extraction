' RX-5 ROM extractor
' By Saga Games
' https://sagagames.de/
' BSD license

#Define ROM_FILE "a.bin"
'#Define ROM_FILE "M5M23C100-513P-4.bin"
#Define DUMP_SAMPLES

#Include "vbcompat.bi"

#Macro BSWAP32(a)
Asm
	mov eax, [a]
	bswap eax
	mov [a], eax
End Asm
#EndMacro

#Define BSWAP16(a) a = (((a And &HFF) Shl 8) Or ((a And &HFF00) Shr 8))

Function Checksum(Bytes As Byte Ptr, BLen As Integer) As Integer
	Dim As UShort chsum = 0, origsum = 0
	For i As Integer = 0 To BLen - 3
		chsum += *Cast(UByte Ptr, @Bytes[i])
	Next
	origsum = *Cast(UShort Ptr, @Bytes[BLen - 2])
	BSWAP16(origsum)
	If(origsum = chsum) Then Return chsum Else Return -1 
End Function

Type filehead Field = 1
	Dim As UInteger nulls
	Dim As UByte unknown
	Dim As UByte numsamples
End Type

Type smphead Field = 1
	Dim As UShort unknown1	' $02, $78
	Dim As UByte flags		' 01 = 12-bit, 00 = 8-bit
	Dim As UByte unknown2   ' AND 01 is loop on??? what is AND $40?
	Dim As UShort offset
	Dim As UShort unknown3
	Dim As UInteger endoffset
	Dim As UShort loopend
	Dim As ZString * 12 unknown4
	Dim As ZString * 6 sname
End Type

Assert(SizeOf(smphead) = 32)

Open ROM_FILE For Binary As #1

Dim As filehead  fhead
Get #1, 1, fhead

Dim As smphead sheader
Dim As UInteger fulloffset, fulllength, loopend

Print fhead.numsamples & " samples..."
Print

For i As Integer = 1 To fhead.numsamples
	Get #1, , sheader

	fulloffset = (sheader.offset And &H0FFF) Shl 8 ' the low 8 bits of the offset are not stored (this allows 24-bit addressing)

	fulllength = sheader.endoffset
	BSWAP32(fulllength)
	fulllength = ((fulllength And &H0FFFFF00) Shr 8) - fulloffset

	loopend = sheader.loopend
	BSWAP16(loopend)
	loopend = (loopend And &H0FFFFF) - fulloffset

	Print Hex(i, 2) & " " & sheader.sname; ": Offset $" & Hex(fulloffset, 5) & " - Length $" & Hex(fulllength, 5) & " - LEnd $" & Hex(loopend, 5) & " - ";
	If(sheader.flags And &H01) Then Print "12-bit" Else Print "8-bit"
	If(sheader.flags > 1) Then Print "UNKNOWN FLAG ERROR!"

	' dumping samples
	#Ifdef DUMP_SAMPLES
	Dim As Integer oldpos = Loc(1)
	Kill Trim(sheader.sname)
	Open Trim(sheader.sname) For Binary As #2
	Dim As String buffer = Space(fulllength)
	Get #1, fulloffset + 1,buffer
	Put #2, 1,buffer
	Close #2
	Seek #1, oldpos + 1
	#EndIf
Next

Print

Dim As Integer romChSum
Dim As Byte Ptr romData = Allocate(Lof(1))
Get #1, 1, *romData, Lof(1)

#Macro CalcChecksum(sName, nLen)
romChSum = Checksum(romData, nLen)
If(romChSum <> - 1) Then Print sName & " checksum OK (" & Hex(romChSum) & ")" Else Print sName & " checksum error!" 
#EndMacro

CalcChecksum("Header", 1024)
CalcChecksum("ROM", Lof(1))

Close #1

Sleep

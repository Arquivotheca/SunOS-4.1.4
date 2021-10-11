(* Copyright (c) 1979 Regents of the University of California *)

const
sccsid = '@(#)unixio.h 1.1 10/31/94';

type
fileptr = record
	cnt :integer
	end;

function TELL(
var	fptr :text)
{returns} :fileptr;

  external;

procedure SEEK(
 var	fptr :text;
 var	cnt :fileptr);

  external;

procedure APPEND(
 var	fptr :text);

   external;

function GETFILE(var f: text): integer;	(* returns stdio buffer of f *)
   external;

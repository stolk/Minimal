#!/usr/bin/python3

import sys
import functools

def read_materials(fname) :
	m = {}
	f = open(fname,'r')
	assert(f)
	lines = f.readlines()
	f.close()
	mname = ""
	for line in lines:
		if line[:7] == "newmtl " :
			mname = line[7:].strip()
		if line[:3] == "Kd " :
			rgb = [ float(x) for x in line[3:].split(' ')]
			m[mname]=rgb
	return m


def read_verts(lines) :
	verts = []
	for line in lines:
		if line[:2] == "v " :
			xyz = [ float(x) for x in line[2:].split(' ') ]
			verts.append(xyz)
	return verts


def read_normals(lines) :
	normals = []
	for line in lines:
		if line[:3] == "vn " :
			xyz = [ float(x) for x in line[3:].split(' ') ]
			normals.append(xyz)
	return normals


def read_groups(lines) :
	groups = {}
	gname = ""
	facesv = []
	facesn = []
	for line in lines :
		if line[:2] == "g " :
			if gname :
				groups[ gname ] = ( facesv, facesn )
				facesv = []
				facesn = []
			gname = line[2:].strip()
		if line[:2] == "f " :
			vindices = [ int(x.split('/')[0]) for x in line[2:].split(' ') ]
			nindices = [ int(x.split('/')[2]) for x in line[2:].split(' ') ]
			facesv.append( vindices )
			facesn.append( nindices )
	if ( len(facesv) ) :
		groups[ gname ] = ( facesv, facesn )

	return groups


def calc_triangle_counts(groups) :
	counts = []
	for gname in groups.keys() :
		facesv = groups[gname][0]
		count = 0
		for face in facesv:
			count += ( len(face) - 2 )
		counts.append( count )
	return counts


def calc_line_count(groups) :
	count = 0
	for gname in groups.keys() :
		facesv = groups[gname][0]
		for face in facesv:
			count += len(face)
	return count


def output_verts(f, bname, verts) :
	f.write( "static float verts_%s[%d][3]={\n" % ( bname, len( verts ) ) )
	for vert in verts :
		f.write( "  %f,%f,%f,\n" % ( vert[0],vert[1],vert[2] ) )
	f.write( "};\n" )


def output_colours(f, bname, groups, materials) :
	f.write( "static float colrs_%s[%d][4]={\n" % ( bname, len(groups) ) )
	for gname in groups.keys() :
		cname = gname.split('_')[-1]
		rgb = materials[cname]
		f.write("  %f,%f,%f,1, // %s\n" % ( rgb[0], rgb[1], rgb[2], cname ) )
	f.write( "};\n" )


def output_triangles_indexed(f, bname, groups, counts) :
	totalcount = 0
	f.write( "static unsigned short group_sizes_%s[%d]={\n" % ( bname, len(counts) ) )
	i = 0
	for count in counts :
		f.write( "  %d, // %s \n" % ( count, groups.keys()[ i ] ) )
		totalcount += count
		i += 1
	f.write( "};\n" )
	f.write( "static unsigned short triangles_%s[%d][3]={\n" % ( bname, totalcount ) )
	for gname in groups.keys() :
		f.write( "  // %s\n" % ( gname, ) )
		facesv = groups[gname][0]
		for face in facesv:
			numtria = len(face) - 2
			for trianr in range(numtria) :
				i0 = 0
				i1 = trianr+1
				i2 = trianr+2
				f.write( "  %d,%d,%d," % ( face[i0]-1, face[i1]-1, face[i2]-1 ) )
			f.write( "\n" )
	f.write( "};\n" )


def output_coloured_triangles(f, bname, groups, nump, verts, norms, materials) :
	f.write( "static const float vdata_%s[%d][3][9]={ // per vertex: x,y,z, nx,ny,nz, r,g,b\n" % ( bname, nump ) )
	for gname in groups.keys() :
		f.write( "  // %s\n" % ( gname, ) )
		cname = gname.split('_')[-1]
		rgb = materials[cname]
		facesv = groups[gname][0]
		facesn = groups[gname][1]
		for i in range(len(facesv)) :
			facev = facesv[i]
			facen = facesn[i]
			numtria = len(facev) - 2
			for trianr in range(numtria) :
				i0 = 0
				i1 = trianr+1
				i2 = trianr+2
				vidx0 = facev[i0]-1
				vidx1 = facev[i1]-1
				vidx2 = facev[i2]-1
				nidx0 = facen[i0]-1
				nidx1 = facen[i1]-1
				nidx2 = facen[i2]-1
				v0 = verts[vidx0]
				v1 = verts[vidx1]
				v2 = verts[vidx2]
				n0 = norms[nidx0]
				n1 = norms[nidx1]
				n2 = norms[nidx2]
				f.write( "  %f,%f,%f, %f,%f,%f, %.01f,%.01f,%.01f,   %f,%f,%f, %f,%f,%f, %.01f,%.01f,%.01f,   %f,%f,%f, %f,%f,%f, %.01f,%.01f,%.01f," % ( v0[0],v0[1],v0[2],  n0[0],n0[1],n0[2], rgb[0],rgb[1],rgb[2],  v1[0],v1[1],v1[2],  n1[0],n1[1],n1[2], rgb[0],rgb[1],rgb[2],  v2[0],v2[1],v2[2],  n2[0],n2[1],n2[2], rgb[0],rgb[1],rgb[2] ) )
			f.write( "\n" )
	f.write( "};\n" )


def output_triangles(f, bname, groups, counts, verts, norms) :
	totalcount = 0
	f.write( "static unsigned short group_sizes_%s[%d]={\n" % ( bname, len(counts) ) )
	i = 0
	for count in counts :
		f.write( "  %d, // %s \n" % ( count, groups.keys()[ i ] ) )
		totalcount += count
		i += 1
	f.write( "};\n" )
	f.write( "static float vdata_%s[%d][3][6]={\n" % ( bname, totalcount ) )
	for gname in groups.keys() :
		f.write( "  // %s\n" % ( gname, ) )
		facesv = groups[gname][0]
		facesn = groups[gname][1]
		for i in range(len(facesv)) :
			facev = facesv[i]
			facen = facesn[i]
			numtria = len(facev) - 2
			for trianr in range(numtria) :
				i0 = 0
				i1 = trianr+1
				i2 = trianr+2
				vidx0 = facev[i0]-1
				vidx1 = facev[i1]-1
				vidx2 = facev[i2]-1
				nidx0 = facen[i0]-1
				nidx1 = facen[i1]-1
				nidx2 = facen[i2]-1
				v0 = verts[vidx0]
				v1 = verts[vidx1]
				v2 = verts[vidx2]
				n0 = norms[nidx0]
				n1 = norms[nidx1]
				n2 = norms[nidx2]
				f.write( "  %f,%f,%f, %f,%f,%f,   %f,%f,%f, %f,%f,%f,   %f,%f,%f, %f,%f,%f," % ( v0[0],v0[1],v0[2],  n0[0],n0[1],n0[2],  v1[0],v1[1],v1[2],  n1[0],n1[1],n1[2],  v2[0],v2[1],v2[2],  n2[0],n2[1],n2[2] ) )
			f.write( "\n" )
	f.write( "};\n" )


def calc_contours(groups, verts, norms) :
	hedges = {}
	for gname in groups.keys() :
		facesv = groups[gname][0]
		facesn = groups[gname][1]
		for i in range(len(facesv)) :
			facev = facesv[i]
			facen = facesn[i]
			nv = len(facev)
			for j in range(nv) :
				i0 = (j+0)
				i1 = (j+1)%nv
				vidx0 = facev[i0]-1
				vidx1 = facev[i1]-1
				nidx0 = facen[i0]-1
				nidx1 = facen[i1]-1
				n0 = norms[nidx0]
				n1 = norms[nidx1]
				hedges[ (vidx0,vidx1) ] = (n0,n1)
	#print "num halfedges:", len(hedges)
	edges = []
	for vs in hedges:
		ns = hedges[ vs ]
		if vs[1] > vs[0] :
			ovs = ( vs[1], vs[0] )
			if ovs in hedges:
				ons = hedges[ ovs ]
				ons = ( ons[1], ons[0] )
				dot0 = ns[0][0]*ons[0][0] + ns[0][1]*ons[0][1] + ns[0][2]*ons[0][2]
				dot1 = ns[1][0]*ons[1][0] + ns[1][1]*ons[1][1] + ns[1][2]*ons[1][2]
				if dot0 < 0.84 or dot1 < 0.84 :
					edges.append( vs )

	#print "num edges:", len(edges)
	return edges



def output_lines_indexed(f, bname, groups, count) :
	f.write( "static unsigned short line_count_%s = %d;\n" % ( bname, count ) );
	f.write( "static unsigned short lines_%s[%d][2]={\n" % ( bname, count ) );
	for gname in groups.keys() :
		f.write( "  // %s\n" % ( gname , ) )
		faces = groups[gname]
		for face in faces:
			numlines = len(face)
			for linenr in range(numlines) :
				i0 = linenr
				i1 = (linenr+1)%numlines
				f.write( "  %d,%d," % ( face[i0]-1, face[i1]-1 ) )
			f.write( "\n" )
	f.write( "};\n" )


def output_edges(f, bname, edges, verts) :
	ecnt = len( edges )
	f.write( "static float edges_%s[%d][6]={\n" % ( bname, ecnt ) )
	for e in edges :
		v0 = verts[ e[0] ]
		v1 = verts[ e[1] ]
		f.write( "  %f,%f,%f, %f,%f,%f,\n" % ( v0[0], v0[1], v0[2], v1[0], v1[1], v1[2] ) )
	f.write( "};\n" )


for arg in sys.argv[ 1: ] :
	bname = arg
	if ".obj" in bname :
		bname = bname.split( '.obj' )[ 0 ]

	objname = bname + ".obj"
	mtlname = bname + ".mtl"
	outname = "geom/geom_" + bname + ".h"
	indexed = False

	materials = read_materials(mtlname)

	f = open(objname,"r")
	assert(f)
	lines = f.readlines()
	f.close()
	verts = read_verts(lines)
	norms = read_normals(lines)
	numv = len(verts)
	groups = read_groups(lines)
	counts = calc_triangle_counts(groups)
	nump = functools.reduce(lambda x,y:x+y, counts)
	edges  = calc_contours(groups, verts, norms)

	f = open(outname,"w")
	assert(f)
	f.write( "const int numv_%s = %d;\n" % ( bname, numv ) )
	f.write( "const int numg_%s = %d;\n" % ( bname, len(groups) ) )
	f.write( "const int nume_%s = %d;\n" % ( bname, len(edges) ) )
	f.write( "const int nump_%s = %d;\n" % ( bname, nump ) )
	#output_colours(f,bname,groups,materials)

	if ( indexed ) :
	  output_verts(f,bname,verts)
	  output_triangles_indexed(f,bname,groups,counts)
	  output_lines_indexed(f,bname,groups,calc_line_count(groups))
	else :
	  output_coloured_triangles(f,bname,groups,nump,verts,norms,materials)
	  output_edges(f,bname,edges,verts)

	f.close()


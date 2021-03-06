/*
This file is part of netcdf-4, a netCDF-like interface for HDF5, or a
HDF5 backend for netCDF, depending on your point of view.

This file handles the nc4 attribute functions.

Remember that with atts, type conversion can take place when writing
them, and when reading them.

Copyright 2003-2005, University Corporation for Atmospheric
Research. See COPYRIGHT file for copying and redistribution
conditions.

$Id: nc4attr.c,v 1.52 2008/06/10 15:38:34 ed Exp $
*/

#include <nc4internal.h>

int nc4typelen(nc_type type);

/* Get or put attribute metadata from our linked list of file
   info. Always locate the attribute by name, never by attnum.
   The mem_type is ignored if data=NULL. */
int
nc4_get_att(int ncid, NC_FILE_INFO_T *nc, int varid, const char *name, 
	    nc_type *xtype, nc_type mem_type, size_t *lenp, 
	    int *attnum, int is_long, void *data) 
{
   NC_GRP_INFO_T *grp;
   NC_HDF5_FILE_INFO_T *h5;
   NC_ATT_INFO_T *att;
   int my_attnum = -1;
   int need_to_convert = 0;
   int range_error = NC_NOERR;
   void *bufr = NULL;
   size_t type_size;
   char norm_name[NC_MAX_NAME + 1];
   int i;
   int retval = NC_NOERR;

   if (attnum)
      my_attnum = *attnum;
   assert(nc && nc->nc4_info);

   LOG((3, "nc4_get_att: ncid 0x%x varid %d name %s attnum %d mem_type %d", 
	ncid, varid, name, my_attnum, mem_type));

   /* Find info for this file and group, and set pointer to each. */
   h5 = nc->nc4_info;
   if (!(grp = nc4_rec_find_grp(h5->root_grp, (ncid & GRP_ID_MASK))))
      return NC_EBADGRPID;      

   /* Normalize name. */
   if ((retval = nc4_normalize_name(name, norm_name)))
      return retval;

   /* Find the attribute, if it exists. If we don't find it, we are
      major failures. */
   if ((retval = nc4_find_grp_att(grp, varid, norm_name, my_attnum, &att)))
      return retval;
   
   /* If mem_type is NC_NAT, it means we want to use the attribute's
    * file type as the mem type as well. */
   if (mem_type == NC_NAT)
      mem_type = att->xtype;

   /* If the attribute is NC_CHAR, and the mem_type isn't, or vice
    * versa, that's a freakish attempt to convert text to
    * numbers. Some pervert out there is trying to pull a fast one!
    * Send him an NC_ECHAR error...*/
   if (data && att->len &&
       ((att->xtype == NC_CHAR && mem_type != NC_CHAR) ||
	(att->xtype != NC_CHAR && mem_type == NC_CHAR)))
      return NC_ECHAR; /* take that, you freak! */

   /* Copy the info. */
   if (lenp)
      *lenp = att->len;
   if (xtype)
      *xtype = att->xtype;
   if (attnum)
      *attnum = att->attnum;

   /* Zero len attributes are easy to read! */
   if (!att->len)
      return NC_NOERR;

   /* Later on, we will need to know the size of this type. */
   if ((retval = nc4_get_typelen_mem(h5, mem_type, is_long, &type_size)))
      return retval;

   /* We may have to convert data. Treat NC_CHAR the same as
    * NC_UBYTE. If the mem_type is NAT, don't try any conversion - use
    * the attribute's type. */
   if (data && att->len && mem_type != att->xtype &&
       mem_type != NC_NAT &&
       !(mem_type == NC_CHAR && 
	 (att->xtype == NC_UBYTE || att->xtype == NC_BYTE)))
   {
      need_to_convert++;
      if (!(bufr = nc_malloc((size_t)(att->len * type_size))))
	 return NC_ENOMEM;
      if ((retval = nc4_convert_type(att->data, bufr, att->xtype, 
				     mem_type, (size_t)att->len, &range_error, 
				     NULL, (h5->cmode & NC_CLASSIC_MODEL), 0, is_long)))
	 BAIL(retval);

      /* For strict netcdf-3 rules, ignore erange errors between UBYTE
       * and BYTE types. */
      if ((h5->cmode & NC_CLASSIC_MODEL) &&
	  (att->xtype == NC_UBYTE || att->xtype == NC_BYTE) &&
	  (mem_type == NC_UBYTE || mem_type == NC_BYTE) &&
	  range_error)
	 range_error = 0;
   }
   else
   {
      bufr = att->data;
   }

   /* If the caller wants data, copy it for him. If he hasn't
      allocated enough memory for it, he will burn in segmantation
      fault hell, writhing with the agony of undiscovered memory
      bugs! */
   if (data)
   {
      if (att->vldata)
      {
	 size_t base_typelen = type_size;
	 hvl_t *vldest = data;
	 NC_TYPE_INFO_T *type;
	 if ((retval = nc4_find_type(h5, att->xtype, &type)))
	    return retval;
	 for (i = 0; i < att->len; i++)
	 {
	    vldest[i].len = att->vldata[i].len;
	    if (!(vldest[i].p = nc_malloc(vldest[i].len * base_typelen)))
	       BAIL(NC_ENOMEM);
	    memcpy(vldest[i].p, att->vldata[i].p, vldest[i].len * base_typelen);
	 }
      }
      else if (att->stdata)
      {
	 char **stdata = (char **)data;
	 for (i = 0; i < att->len; i++)
	 {
	    if (!(*stdata = malloc(strlen(att->stdata[i]) + 1)))
	       BAIL(NC_ENOMEM);
	    strcpy(*stdata, att->stdata[i]);
	    stdata++;
	 }
      }
      else
      {
	 /* For long types, we need to handle this special... */
	 if (is_long && att->xtype == NC_INT)
	 {
	    long *lp = data;
	    int *ip = bufr;
	    for (i = 0; i < att->len; i++)
	       *lp++ = *ip++;
	 }
	 else
	    memcpy(data, bufr, (size_t)(att->len * type_size));
      }
   }

 exit:
   if (need_to_convert) nc_free(bufr);
   if (retval)
      return retval;
   if (range_error)
      return NC_ERANGE;
   return NC_NOERR;
}

/* Put attribute metadata into our global metadata. */
int
nc4_put_att(int ncid, NC_FILE_INFO_T *nc, int varid, const char *name, 
	    nc_type file_type, nc_type mem_type, size_t len, int is_long, 
	    const void *data)
{
   NC_GRP_INFO_T *grp; 
   NC_HDF5_FILE_INFO_T *h5;
   NC_VAR_INFO_T *var = NULL;
   NC_ATT_INFO_T *att, **attlist = NULL, *varatt;
   NC_TYPE_INFO_T *type = NULL;
   char norm_name[NC_MAX_NAME + 1];
   int new_att = 0;
   int retval = NC_NOERR, range_error = 0;
   size_t type_size;
   int i;
   int res;

   if (!name) 
      return NC_EBADNAME;
   assert(nc && nc->nc4_info);

   LOG((1, "nc4_put_att: ncid 0x%x varid %d name %s "
	"file_type %d mem_type %d len %d", ncid, varid,
	name, file_type, mem_type, len));

   /* If len is not zero, then there must be some data. */
   if (len && !data)
      return NC_EINVAL;

   /* Find info for this file and group, and set pointer to each. */
   h5 = nc->nc4_info;
   if (!(grp = nc4_rec_find_grp(h5->root_grp, (ncid & GRP_ID_MASK))))
      return NC_EBADGRPID;      

   /* If the file is read-only, return an error. */
   if (h5->no_write)
     return NC_EPERM;

   /* Check and normalize the name. */
   if ((retval = nc4_check_name(name, norm_name)))
      return retval;

   /* Find att, if it exists. */
   if (varid == NC_GLOBAL)
      attlist = &grp->att;
   else
   {
      for (var = grp->var; var; var = var->next)
	 if (var->varid == varid)
	 {
	    attlist = &var->att;
	    break;
	 }
      if (!var)
	 return NC_ENOTVAR;
   }
   for (att = *attlist; att; att = att->next)
      if (!strcmp(att->name, norm_name))
	 break;

   if (!att)
   {
      /* If this is a new att, require define mode. */
      if (!(h5->flags & NC_INDEF))
      {
	 if (h5->cmode & NC_CLASSIC_MODEL)
	    return NC_EINDEFINE;
	 if ((retval = nc_redef(ncid)))
	    BAIL(retval);
      }
      new_att++;
   }
   else
   {
      /* For an existing att, if we're not in define mode, the len
	 must not be greater than the existing len. */
      if (!(h5->flags & NC_INDEF) && 
	  len * nc4typelen(file_type) > (size_t)att->len * nc4typelen(att->xtype))
      {
	 if (h5->cmode & NC_CLASSIC_MODEL)
	    return NC_EINDEFINE;
	 if ((retval = nc_enddef(ncid)))
	    BAIL(retval);
      }
   }

   /* We must have two valid types to continue. */
   if (file_type == NC_NAT || mem_type == NC_NAT)
      return NC_EBADTYPE;

   /* Get information about this type. */
   if ((retval = nc4_find_type(h5, file_type, &type)))
      return retval;
   if ((retval = nc4_get_typelen_mem(h5, file_type, is_long, &type_size)))
      return retval;

   /* No character conversions are allowed. */
   if (file_type != mem_type && 
       (file_type == NC_CHAR || mem_type == NC_CHAR || 
	file_type == NC_STRING || mem_type == NC_STRING))
      return NC_ECHAR;

   /* Add to the end of the attribute list, if this att doesn't
      already exist. */
   if (new_att)
   {
      LOG((3, "adding attribute %s to the list...", norm_name));
      if ((res = nc4_att_list_add(attlist)))
	 BAIL (res);
      /* Find this att's entry in the list (the last one). */
      for (att=*attlist; att->next; att=att->next)
	 ;
   }

   /* Now fill in the metadata. */
   att->dirty++;
   strcpy(att->name, norm_name);
   att->xtype = file_type;
   att->len = len;
   if (att->prev)
      att->attnum = att->prev->attnum + 1;
   else
      att->attnum = 0;
   if (type)
      att->class = type->class;

   /* If this is the _FillValue attribute, then we will also have to
    * copy the value to the fll_vlue pointer of the NC_VAR_INFO_T
    * struct for this var. (But ignore a global _FillValue
    * attribute). */
   if (!strcmp(att->name, _FillValue) && varid != NC_GLOBAL)
   {
      NC_TYPE_INFO_T *type_info;
      int size;

      /* Fill value must be same type. */
      if (att->xtype != var->xtype)
	 return NC_EINVAL;

      /* If we already wrote to the dataset, then return an error. */
      if (var->written_to)
	 return NC_ELATEFILL;

      /* If fill value hasn't been set, allocate space. Of course,
       * vlens have to be differnt... */
      if ((retval = nc4_get_typelen_mem(grp->file->nc4_info, var->xtype, 0, 
					&type_size)))
	 return retval;
      if ((retval = nc4_find_type(grp->file->nc4_info, var->xtype, &type_info)))
	 BAIL(retval);
      
      /* Already set a fill value? Now I'll have to free the old
       * one. Make up your damn mind, would you? */
      if (var->fill_value)
      {
	 if (type_info && type_info->class == NC_VLEN)
	    if ((retval = nc_free_vlen(var->fill_value)))
	       return retval;
	 free(var->fill_value);
      }

      /* Allocate space for the fill value. */
      if (type_info && type_info->class == NC_VLEN)
	 size = sizeof(hvl_t);
      else
	 size = type_size;

      if (!(var->fill_value = nc_malloc(size)))
	 return NC_ENOMEM;

      /* Copy the fill_value. */
      LOG((4, "Copying fill value into metadata for variable %s", var->name));
      if (type_info && type_info->class == NC_VLEN)
      {
	 nc_vlen_t *in_vlen = (nc_vlen_t *)data, *fv_vlen = (nc_vlen_t *)(var->fill_value);
	 fv_vlen->len = in_vlen->len;
	 if (!(fv_vlen->p = malloc(size * in_vlen->len)))
	    return NC_ENOMEM;
	 memcpy(fv_vlen->p, in_vlen->p, in_vlen->len * size);
      }
      else
	 memcpy(var->fill_value, data, type_size);

      /* Mark the var and all it's atts as dirty, so they get
       * rewritten. */
      var->dirty++;
      for (varatt = var->att; varatt; varatt = varatt->next)
	 varatt->dirty++;
   }

   /* Copy the attribute data, if there is any. VLENs and string
    * arrays have to be handled specially. If this is a fi*/
   if (type && type->class == NC_VLEN && data)
   {
      const hvl_t *vldata1;

      vldata1 = data;
      if (!(att->vldata = nc_malloc(att->len * sizeof(hvl_t))))
	 BAIL(NC_ENOMEM);	 
      for (i = 0; i < att->len; i++)
      {
	 att->vldata[i].len = vldata1[i].len;
	 if (!(att->vldata[i].p = nc_malloc(type_size * att->vldata[i].len)))
	    BAIL(NC_ENOMEM);
	 memcpy(att->vldata[i].p, vldata1[i].p, type_size * att->vldata[i].len);
      }
   }
   else if (file_type == NC_STRING && data)
   {
      LOG((4, "copying array of NC_STRING"));
      if (!(att->stdata = nc_malloc(sizeof(char *) * att->len)))
	 BAIL(NC_ENOMEM);	 
      for (i = 0; i < att->len; i++)
      {
      /* Here I use malloc, not nc_malloc, because although I am in
       * charge of allocating this space with atts that the user puts,
       * the HDF5 library allocates it when the att is read. Since I
       * just have one attribute freeing routine, I can't use
       * nc_malloc here, because I can't get HDF5 to use nc_malloc. */
	 LOG((5, "copying string %d of size %d", i, strlen(((char **)data)[i]) + 1));
	 if (!(att->stdata[i] = malloc(strlen(((char **)data)[i]) + 1)))
	    BAIL(NC_ENOMEM);
	 strcpy(att->stdata[i], ((char **)data)[i]);
      }
   }
   else
   {
      /* Data types are like religions, in that one can convert.  */
      if (att->len)
      {
	 if (!new_att)
	    nc_free (att->data);
	 if (!(att->data = nc_malloc(att->len * type_size)))
	    BAIL(NC_ENOMEM);
	 if (type)
	 {
	    /* Just copy the data... */
	    if (type->class == NC_OPAQUE || type->class == NC_COMPOUND || type->class == NC_ENUM)
	       memcpy(att->data, data, len * type_size);
	    else
	       LOG((0, "nc4_put_att: unknown type."));
	 }
	 else
	 {
	    if ((retval = nc4_convert_type(data, att->data, mem_type, file_type, 
					   len, &range_error, NULL, 
					   (h5->cmode & NC_CLASSIC_MODEL), is_long, 0)))
	       BAIL(retval);
	 }
      }
   }
   att->dirty = 1;
   att->created = 0;

 exit:
   /* If there was an error return it, otherwise return any potential
      range error value. If none, return NC_NOERR as usual.*/
   if (retval)      
      return retval;
   if (range_error)
      return NC_ERANGE;
   return NC_NOERR;
}

/* Learn about an att. All the nc4 nc_inq_ functions just call
 * add_meta_get to get the metadata on an attribute. */
int
nc_inq_att(int ncid, int varid, const char *name, nc_type *xtypep, size_t *lenp)
{
   NC_FILE_INFO_T *nc;

   LOG((2, "nc_inq_att: ncid 0x%x varid %d name %s", ncid, varid, name));

   /* Find metadata. */
   if (!(nc = nc4_find_nc_file(ncid)))
      return NC_EBADID;

   /* Handle netcdf-3 files. */
   if (!nc->nc4_info)
      return nc3_inq_att(nc->int_ncid, varid, name, xtypep, lenp);

   /* Handle netcdf-4 files. */
   return nc4_get_att(ncid, nc, varid, name, xtypep, NC_UBYTE, lenp, NULL, 0, NULL);
}

/* Learn an attnum, given a name. */
int 
nc_inq_attid(int ncid, int varid, const char *name, int *attnump)
{
   NC_FILE_INFO_T *nc;

   LOG((2, "nc_inq_attid: ncid 0x%x varid %d name %s", ncid, varid, name));

   /* Find metadata. */
   if (!(nc = nc4_find_nc_file(ncid)))
      return NC_EBADID;

   /* Handle netcdf-3 files. */
   if (!nc->nc4_info)
      return nc3_inq_attid(nc->int_ncid, varid, name, attnump);

   /* Handle netcdf-4 files. */
   return nc4_get_att(ncid, nc, varid, name, NULL, NC_UBYTE, 
		      NULL, attnump, 0, NULL);
}

/* Find an att's type. */
int 
nc_inq_atttype(int ncid, int varid, const char *name, 
		nc_type *xtypep)
{
   return nc_inq_att(ncid, varid, name, xtypep, NULL);
}

/* Find the len of the att's data. */
int 
nc_inq_attlen(int ncid, int varid, const char *name, size_t *lenp)
{
   return nc_inq_att(ncid, varid, name, NULL, lenp);
}

/* Given an attnum, find the att's name. */
int
nc_inq_attname(int ncid, int varid, int attnum, char *name)
{
   NC_FILE_INFO_T *nc;
   NC_ATT_INFO_T *att;
   int retval = NC_NOERR;

   LOG((2, "nc_inq_attname: ncid 0x%x varid %d attnum %d", 
	ncid, varid, attnum));

   /* Find metadata. */
   if (!(nc = nc4_find_nc_file(ncid)))
      return NC_EBADID;

   /* Handle netcdf-3 files. */
   if (!nc->nc4_info)
      return nc3_inq_attname(nc->int_ncid, varid, attnum, name);

   /* Handle netcdf-4 files. */
   if ((retval = nc4_find_nc_att(ncid, varid, NULL, attnum, &att)))
      return retval;

   /* Get the name. */
   if (name)
      strcpy(name, att->name);

   return NC_NOERR;
}

/* Copy an attribute from one open file to another.

   Special programming challenge: this function must work even if one
   of the other of the files is a netcdf version 1.0 file (i.e. not
   HDF5). So only use top level netcdf api functions. 

   From the netcdf-3 docs: The output netCDF dataset should be in
   define mode if the attribute to be copied does not already exist
   for the target variable, or if it would cause an existing target
   attribute to grow.
*/
int
nc_copy_att(int ncid_in, int varid_in, const char *name, 
	    int ncid_out, int varid_out)
{
   nc_type xtype;
   size_t len;
   void *data=NULL;
   int res;

   LOG((2, "nc_copy_att: ncid_in 0x%x varid_in %d name %s", 
	ncid_in, varid_in, name));

   /* Find out about the attribute and allocate memory for the
      data. */
   if ((res = nc_inq_att(ncid_in, varid_in, 
			 name, &xtype, &len)))
      return res;
   if (len)
     if (!(data = nc_malloc(len * nc4typelen(xtype))))
       return NC_ENOMEM;

   if (ncid_in == ncid_out && varid_in == varid_out)
      return NC_NOERR;

   /* Based on the type, read and write it. */
   switch (xtype)
   {
   case NC_BYTE:
      res = nc_get_att_schar(ncid_in, varid_in, name, 
			     (signed char *)data);
      if (!res)
	 res = nc_put_att_schar(ncid_out, varid_out, name, 
				NC_BYTE, len, 
				(const signed char *)data);
      break;
   case NC_CHAR:
      res = nc_get_att_text(ncid_in, varid_in, name, 
			    (char *)data);
      if (!res)
	 res = nc_put_att_text(ncid_out, varid_out, name, 
			       len, (char *)data);
      break;
   case NC_SHORT:
      res = nc_get_att_short(ncid_in, varid_in, name, 
			     (short *)data);
      if (!res)
	 res = nc_put_att_short(ncid_out, varid_out, name, 
				NC_SHORT, len, (short *)data);
      break;
   case NC_INT:
      res = nc_get_att_int(ncid_in, varid_in, name, 
			   (int *)data);
      if (!res)
	 res = nc_put_att_int(ncid_out, varid_out, name, 
			      NC_INT, len, (int *)data);
      break;
   case NC_FLOAT:
      res = nc_get_att_float(ncid_in, varid_in, name, 
			     (float *)data);
      if (!res)
	 res = nc_put_att_float(ncid_out, varid_out, name, 
				NC_FLOAT, len, (float *)data);
      break;
   case NC_DOUBLE:
      res = nc_get_att_double(ncid_in, varid_in, name, 
			      (double *)data);
      if (!res)
	 res = nc_put_att_double(ncid_out, varid_out, name, 
				 NC_DOUBLE, len, (double *)data);
      break;
   default:
      res = NC_EBADTYPE;
   }

   if (data) nc_free(data);
   return res;
}

/* I think all atts should be named the exact same thing, to avoid
   confusion! */
int
nc_rename_att(int ncid, int varid, const char *name, 
	      const char *newname)
{
   NC_FILE_INFO_T *nc;
   NC_GRP_INFO_T *grp; 
   NC_HDF5_FILE_INFO_T *h5;
   NC_VAR_INFO_T *var;
   NC_ATT_INFO_T *att, *list;
   char norm_newname[NC_MAX_NAME + 1], norm_name[NC_MAX_NAME + 1];
   hid_t datasetid = 0;
   int retval = NC_NOERR;

   if (!name || !newname)
      return NC_EINVAL;

   LOG((2, "nc_rename_att: ncid 0x%x varid %d name %s newname %s",
	ncid, varid, name, newname));

   /* If the new name is too long, that's an error. */
   if (strlen(newname) > NC_MAX_NAME)
      return NC_EMAXNAME;

   /* Find metadata for this file. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;

   /* Handle netcdf-3 files. */
   if (!h5)
      return nc3_rename_att(nc->int_ncid, varid, name, newname);

   /* If the file is read-only, return an error. */
   if (h5->no_write)
     return NC_EPERM;

   /* Check and normalize the name. */
   if ((retval = nc4_check_name(newname, norm_newname)))
      return retval;

   /* Is norm_newname in use? */
   if (varid == NC_GLOBAL)
   {
      list = grp->att;
   }
   else
   {
      for (var = grp->var; var; var = var->next)
	 if (var->varid == varid)
	 {
	    list = var->att;
	    break;
	 }
      if (!var)
	 return NC_ENOTVAR;
   }
   for (att = list; att; att = att->next)
      if (!strncmp(att->name, norm_newname, NC_MAX_NAME))
	 return NC_ENAMEINUSE;

   /* Normalize name and find the attribute. */
   if ((retval = nc4_normalize_name(name, norm_name)))
      return retval;
   for (att = list; att; att = att->next)
      if (!strncmp(att->name, norm_name, NC_MAX_NAME))
	 break;
   if (!att)
      return NC_ENOTATT;

   /* If we're not in define mode, new name must be of equal or
      less size, if complying with strict NC3 rules. */
   if (!(h5->flags & NC_INDEF) && strlen(norm_newname) > strlen(att->name) &&
       (h5->cmode & NC_CLASSIC_MODEL))
      return NC_ENOTINDEFINE;

   /* Delete the original attribute, if it exists in the HDF5 file. */
   if (att->created)
   {
      if (varid == NC_GLOBAL)
      {
	 retval = nc4_delete_hdf5_att(grp->hdf_grpid, att->name);
      }
      else
      {
	 if ((retval = nc4_open_var_grp(grp, varid, &datasetid)))
	    return retval;
	 retval = nc4_delete_hdf5_att(datasetid, att->name);
	 if (H5Dclose(datasetid) < 0)
	    BAIL(NC_EHDFERR);
	 if (retval)
	    return retval;
      }
   }

   /* Copy the new name into our metadata. */
   strcpy(att->name, norm_newname);
   att->dirty = 1;

  exit:
   if (datasetid > 0) H5Dclose(datasetid);   
   return retval;
}

/* Delete an att. Rub it out. Push the button on it. Liquidate
   it. Bump it off. Take it for a one-way ride. Terminate it. Drop the
   bomb on it. You get the idea. 
   Ed Hartnett, 10/1/3 
*/
int
nc_del_att(int ncid, int varid, const char *name)
{
   NC_FILE_INFO_T *nc;
   NC_GRP_INFO_T *grp; 
   NC_HDF5_FILE_INFO_T *h5;
   NC_ATT_INFO_T *att, *natt;
   NC_VAR_INFO_T *var;
   NC_ATT_INFO_T **attlist = NULL;
   hid_t locid = 0, datasetid = 0;
   int retval = NC_NOERR;

   if (!name)
      return NC_EINVAL;

   LOG((2, "nc_del_att: ncid 0x%x varid %d name %s",
	ncid, varid, name));
   
   /* Find metadata for this file. */
   if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
      return retval;

   /* Handle netcdf-3 files. */
   if (!h5)
      return nc3_del_att(nc->int_ncid, varid, name);   

   assert(h5 && grp);

   /* If the file is read-only, return an error. */
   if (h5->no_write)
      return NC_EPERM;

   /* If it's not in define mode, forget it. */
   if (!(h5->flags & NC_INDEF))
   {
      if (h5->cmode & NC_CLASSIC_MODEL)
	 return NC_ENOTINDEFINE;
      if ((retval = nc_redef(ncid)))
	 BAIL(retval);
   }

   /* Get either the global or a variable attribute list. Also figure
      out the HDF5 location it's attached to. */
   if (varid == NC_GLOBAL)
   {
      attlist = &grp->att;
      locid = grp->hdf_grpid;
   }
   else
   {
      for(var = grp->var; var; var = var->next)
      {
	 if (var->varid == varid)
	 {
	    attlist = &var->att;
	    break;
	 }
      }
      if (!var)
	 return NC_ENOTVAR;
      if (var->created)
      {
	 if ((retval = nc4_open_var_grp(grp, var->varid, &datasetid)))
	    return retval;
	 locid = datasetid;
      }
   }

   /* Now find the attribute by name or number. */
   for (att = *attlist; att; att = att->next)
      if (!strcmp(att->name, name))
	 break;

   /* If att is NULL, we couldn't find the attribute. */
   if (!att) 
      BAIL_QUIET(NC_ENOTATT);
   
   /* Delete it from the HDF5 file, if it's been created. */
   if (att->created)
      if(H5Adelete(locid, att->name) < 0)
	 BAIL(NC_EATTMETA);

   /* Renumber all following attributes. */
   for (natt = att->next; natt; natt = natt->next)
      natt->attnum--;

   /* Delete this attribute from this list. */
   nc4_att_list_del(attlist, att);
   
 exit:
   if (datasetid > 0) H5Dclose(datasetid);
   return retval;
}

/* Write an attribute with type conversion. */
int
nc4_put_att_tc(int ncid, int varid, const char *name, nc_type file_type, 
	       nc_type mem_type, int mem_type_is_long, size_t len, 
	       const void *op)
{
   NC_FILE_INFO_T *nc;

   if (!name || strlen(name) > NC_MAX_NAME)
      return NC_EBADNAME;

   LOG((3, "nc4_put_att_tc: ncid 0x%x varid %d name %s file_type %d "
	"mem_type %d len %d", ncid, varid, name, file_type, mem_type, len));

   /* The length needs to be positive (cast needed for braindead
      systems with signed size_t). */
   if((unsigned long) len > X_INT_MAX) 
      return NC_EINVAL;

   /* Find metadata. */
   if (!(nc = nc4_find_nc_file(ncid)))
      return NC_EBADID;

   /* Handle netcdf-3 files. */
   if (!nc->nc4_info)
   {
      if (mem_type == NC_UBYTE)
	 mem_type = NC_BYTE;
      switch(mem_type)
      {
	 case NC_NAT:
	    return nc3_put_att(nc->int_ncid, varid, name, file_type, 
			      len, op);
	 case NC_BYTE:
	    return nc3_put_att_schar(nc->int_ncid, varid, name, 
				     file_type, len, op);
	 case NC_CHAR:
	    return nc3_put_att_text(nc->int_ncid, varid, name, 
				    len, op);
	 case NC_SHORT:
	    return nc3_put_att_short(nc->int_ncid, varid, name, 
				     file_type, len, op);
	 case NC_INT:
	    if (mem_type_is_long)
	       return nc3_put_att_long(nc->int_ncid, varid, name, 
				       file_type, len, op);
	    else
	       return nc3_put_att_int(nc->int_ncid, varid, name, 
				      file_type, len, op);
	 case NC_FLOAT:
	    return nc3_put_att_float(nc->int_ncid, varid, name, 
				     file_type, len, op);
	 case NC_DOUBLE:
	    return nc3_put_att_double(nc->int_ncid, varid, name, 
				      file_type, len, op);
	 default:
	    return NC_EBADTYPE;
      }
   }

   /* Otherwise, handle things the netcdf-4 way. */
   return nc4_put_att(ncid, nc, varid, name, file_type, mem_type, len, 
		      mem_type_is_long, op);
}

/* Write an attribute of any type. */
EXTERNL int
nc_put_att(int ncid, int varid, const char *name, nc_type xtype, 
	   size_t len, const void *op)
{
   return nc4_put_att_tc(ncid, varid, name, xtype, xtype, 0, len, op);
}

/* Read an attribute of any type, with type conversion. This may be
 * called by any of the nc_get_att_* functions. */
int
nc4_get_att_tc(int ncid, int varid, const char *name, nc_type mem_type, 
	       int mem_type_is_long, void *ip)
{
   NC_FILE_INFO_T *nc;

   LOG((3, "nc4_get_att_tc: ncid 0x%x varid %d name %s mem_type %d", 
	ncid, varid, name, mem_type));

   /* Find metadata. */
   if (!(nc = nc4_find_nc_file(ncid)))
      return NC_EBADID;

   /* Handle netcdf-3 files. */
   if (!nc->nc4_info)
   {
      /* NetCDF-3 doesn't know UBYTE from a hole in the ground. It
       * only knows about BYTE. */
      if (mem_type == NC_UBYTE)
	 mem_type = NC_BYTE;
      switch(mem_type)
      {
	 case NC_NAT:
	    return nc3_get_att(nc->int_ncid, varid, name, ip);
	 case NC_BYTE:
	    return nc3_get_att_schar(nc->int_ncid, varid, name, ip);
	 case NC_CHAR:
	    return nc3_get_att_text(nc->int_ncid, varid, name, ip);
	 case NC_SHORT:
	    return nc3_get_att_short(nc->int_ncid, varid, name, ip);
	 case NC_INT:
	    if (mem_type_is_long)
	       return nc3_get_att_long(nc->int_ncid, varid, name, ip);
	    else
	       return nc3_get_att_int(nc->int_ncid, varid, name, ip);
	 case NC_FLOAT:
	    return nc3_get_att_float(nc->int_ncid, varid, name, ip);
	 case NC_DOUBLE:
	    return nc3_get_att_double(nc->int_ncid, varid, name, ip);
	 default:
	    return NC_EBADTYPE;
      }
   }

   return nc4_get_att(ncid, nc, varid, name, NULL, mem_type, 
		      NULL, NULL, mem_type_is_long, ip);
}

/* Read an attribute of any type, with NO type conversion. */
EXTERNL int
nc_get_att(int ncid, int varid, const char *name, void *ip)
{
   return nc4_get_att_tc(ncid, varid, name, NC_NAT, 0, ip);
}

/* Put an ascii string into an att. In HDF5 this translates to a
 * scalar attribute of type H5T_C_S1. */
int
nc_put_att_text(int ncid, int varid, const char *name,
		 size_t len, const char *op)
{
   return nc4_put_att_tc(ncid, varid, name, NC_CHAR, NC_CHAR, 0, 
			 len, op);
}

/* Get an ascii string from an att. */
int
nc_get_att_text(int ncid, int varid, const char *name, char *ip)
{
   return nc4_get_att_tc(ncid, varid, name, NC_CHAR, 0, ip);
}

/* Get an unsigned char from an att, doing type conversion as
 * needed! */
int
nc_put_att_uchar(int ncid, int varid, const char *name, nc_type xtype,
		  size_t len, const unsigned char *op)
{
   if (xtype == NC_NAT)
      return NC_EBADTYPE;
   return nc4_put_att_tc(ncid, varid, name, xtype, NC_UBYTE, 
			 0, len, op);
}

int
nc_get_att_uchar(int ncid, int varid, const char *name, 
		 unsigned char *ip)
{
   return nc4_get_att_tc(ncid, varid, name, NC_UBYTE, 0, ip);
}

int
nc_put_att_schar(int ncid, int varid, const char *name, nc_type xtype,
		 size_t len, const signed char *op)
{
   if (xtype == NC_NAT)
      return NC_EBADTYPE;
   return nc4_put_att_tc(ncid, varid, name, xtype, NC_BYTE, 
			 0, len, op);
}

int
nc_get_att_schar(int ncid, int varid, const char *name, 
		  signed char *ip)
{
   return nc4_get_att_tc(ncid, varid, name, NC_BYTE, 0, ip);
}

int
nc_put_att_short(int ncid, int varid, const char *name, 
		 nc_type xtype, size_t len, const short *op)
{
   if (xtype == NC_NAT)
      return NC_EBADTYPE;
   return nc4_put_att_tc(ncid, varid, name, xtype, NC_SHORT, 
			 0, len, op);
}

/* I always say, why get short when you can get tall!! */
int
nc_get_att_short(int ncid, int varid, const char *name, short *ip)
{
   return nc4_get_att_tc(ncid, varid, name, NC_SHORT, 0, ip);
}

int
nc_put_att_int(int ncid, int varid, const char *name, nc_type xtype,
	       size_t len, const int *op)
{
   if (xtype == NC_NAT)
      return NC_EBADTYPE;
   return nc4_put_att_tc(ncid, varid, name, xtype, NC_INT, 
			 0, len, op);
}

int
nc_get_att_int(int ncid, int varid, const char *name, int *ip)
{
   return nc4_get_att_tc(ncid, varid, name, NC_INT, 0, ip);
}

int
nc_put_att_long(int ncid, int varid, const char *name, 
		nc_type xtype, size_t len, const long *op)
{
   if (xtype == NC_NAT)
      return NC_EBADTYPE;
   return nc4_put_att_tc(ncid, varid, name, xtype, NC_INT, 
			 1, len, op);
}

/* Get a long attribute's data. */
int
nc_get_att_long(int ncid, int varid, const char *name, long *ip)
{
   return nc4_get_att_tc(ncid, varid, name, NC_INT, 1, ip);
}

/* In the event of an emergency, please use this attribute as a
   floatation device. */
int
nc_put_att_float(int ncid, int varid, const char *name, nc_type xtype,
		 size_t len, const float *op)
{
   if (xtype == NC_NAT)
      return NC_EBADTYPE;
   return nc4_put_att_tc(ncid, varid, name, xtype, NC_FLOAT, 
			 0, len, op);
}

int
nc_get_att_float(int ncid, int varid, const char *name, float *ip)
{
   return nc4_get_att_tc(ncid, varid, name, NC_FLOAT, 0, ip);
}

/* This is how you store the number of atomic particles in the
   universe. */
int
nc_put_att_double(int ncid, int varid, const char *name, nc_type xtype,
		   size_t len, const double *op)
{
   if (xtype == NC_NAT)
      return NC_EBADTYPE;
   return nc4_put_att_tc(ncid, varid, name, xtype, NC_DOUBLE, 
			 0, len, op);
}

/* Double your pleasure with a double floating point att. */
int
nc_get_att_double(int ncid, int varid, const char *name, double *ip)
{
   return nc4_get_att_tc(ncid, varid, name, NC_DOUBLE, 0, ip);
}

int
nc_put_att_ubyte(int ncid, int varid, const char *name, nc_type xtype,
		 size_t len, const unsigned char *op)
{
   return nc4_put_att_tc(ncid, varid, name, xtype, NC_UBYTE, 0, 
			 len, (void *)op);
}

int
nc_get_att_ubyte(int ncid, int varid, const char *name, 
		 unsigned char *ip)
{
   return nc4_get_att_tc(ncid, varid, name, NC_UBYTE, 0, ip);
}

int
nc_put_att_ushort(int ncid, int varid, const char *name, nc_type xtype,
		  size_t len, const unsigned short *op)
{
   return nc4_put_att_tc(ncid, varid, name, xtype, NC_USHORT, 0,
			 len, (void *)op);
}

int
nc_get_att_ushort(int ncid, int varid, const char *name, 
		  unsigned short *ip)
{
   return nc4_get_att_tc(ncid, varid, name, NC_USHORT, 0, ip);
}

int
nc_put_att_uint(int ncid, int varid, const char *name, nc_type xtype,
		size_t len, const unsigned int *op)
{
   return nc4_put_att_tc(ncid, varid, name, xtype, NC_UINT, 0,
			 len, (void *)op);
}

int
nc_get_att_uint(int ncid, int varid, const char *name, unsigned int *ip)
{
   return nc4_get_att_tc(ncid, varid, name, NC_UINT, 0, ip);
}

int
nc_put_att_longlong(int ncid, int varid, const char *name, nc_type xtype,
		    size_t len, const long long *op)
{
   return nc4_put_att_tc(ncid, varid, name, xtype, NC_INT64, 
			 0, len, (void *)op);
}

int
nc_get_att_longlong(int ncid, int varid, const char *name, 
		    long long *ip)
{
   return nc4_get_att_tc(ncid, varid, name, NC_INT64, 0, ip);
}

int
nc_put_att_ulonglong(int ncid, int varid, const char *name, nc_type xtype,
		     size_t len, const unsigned long long *op)
{
   return nc4_put_att_tc(ncid, varid, name, xtype, NC_UINT64, 0,
			 len, (void *)op);
}

int
nc_get_att_ulonglong(int ncid, int varid, const char *name, 
		     unsigned long long *ip)
{
   return nc4_get_att_tc(ncid, varid, name, NC_UINT64, 0, ip);
}

int
nc_put_att_string(int ncid, int varid, const char *name,
		  size_t len, const char **op)
{
   return nc4_put_att_tc(ncid, varid, name, NC_STRING, NC_STRING, 0,
			 len, (void *)op);
}

int
nc_get_att_string(int ncid, int varid, const char *name, 
		  char **ip)
{
   return nc4_get_att_tc(ncid, varid, name, NC_STRING, 0, ip);
}

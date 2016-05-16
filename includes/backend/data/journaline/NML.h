/*
 *
 * This file is part of the 'NewsService Journaline(R) Decoder'
 *
 * Copyright (c) 2003, 2001-2014 by Fraunhofer IIS, Erlangen, Germany
 *
 * --------------------------------------------------------------------
 *
 * For NON-COMMERCIAL USE,
 * the 'NewsService Journaline(R) Decoder' is free software;
 * you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The 'NewsService Journaline(R) Decoder' is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the 'NewsService Journaline(R) Decoder';
 * if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * If you use this software in a project with user interaction, please
 * provide the following text to the user in an appropriate place:
 * "Features NewsService Journaline(R) decoder technology by
 * Fraunhofer IIS, Erlangen, Germany.
 * For more information visit http://www.iis.fhg.de/dab"
 *
 * --------------------------------------------------------------------
 *
 * To use the 'NewsService Journaline(R) Decoder' software for
 * COMMERCIAL purposes, please contact Fraunhofer IIS for a
 * commercial license (see below for contact information)!
 *
 * --------------------------------------------------------------------
 *
 * Contact:
 *   Fraunhofer IIS, Department 'Broadcast Applications'
 *   Am Wolfsmantel 33, 91058 Erlangen, Germany
 *   http://www.iis.fraunhofer.de/dab
 *   mailto:bc-info@iis.fraunhofer.de
 *
 */

//////////////////////////////////////////////////////////////////////////////
/// @brief NML class
///
/// handling of NML (news markup language for Journaline) data
///
/// @file       NML.h
/// @author     Michael Reichenbächer <rbr@iis.fraunhofer.de>
///
/// $Id: NML.h,v 1.3 2008/12/26 20:04:53 jcable Exp $
///
/// Module:     Journaline(R)
///
/// Copyright:  (C) 2003-2001-2014 by Fraunhofer IIS-A, IT-Services, Erlangen
///
/// Compiler:   gcc version 3.3 20030226 (prerelease)
///             Microsoft Visual C++ .NET
///
//////////////////////////////////////////////////////////////////////////////


#ifndef _NML_H_
#define _NML_H_


#ifdef _MSC_VER
#pragma warning(push,3)
#pragma warning(disable:4018)
#pragma warning(disable:4100)
#pragma warning(disable:4514)
#pragma warning(disable:4786)
#endif

#include <string>
#include <vector>
#include <ios>

#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(disable:4786)
#pragma warning(disable:4514)
#endif

// utility functions
std::string HexDump(const unsigned char *p,
                    unsigned int len,
                    unsigned int bytes_per_line = 16);
std::string HexDump(const char *p,
                    unsigned int len,
                    unsigned int bytes_per_line = 16);



/// interface for NML escape code handlers
class NMLEscapeCodeHandler
{
public:
    virtual bool Convert(std::string & dest,
                         const std::string & src) const = 0;
    virtual ~NMLEscapeCodeHandler() {}
};


/// handle NML escape codes by removing them
class RemoveNMLEscapeSequences : public NMLEscapeCodeHandler
{
public:
    RemoveNMLEscapeSequences();
    virtual ~RemoveNMLEscapeSequences();

    bool Convert(std::string & dest,
                 const std::string & src) const;
};


/// handle NML escape codes by converting them to HTML codes
class NMLEscapeSequences2HTML : public NMLEscapeCodeHandler
{
public:
    NMLEscapeSequences2HTML();
    virtual ~NMLEscapeSequences2HTML();

    bool Convert(std::string & dest,
                 const std::string & src) const;
};



/// NML (news markup language for Journaline)
class NML
{
    friend class NMLFactory;

public:
    //-------------------- constants --------------------
    static const unsigned int NML_MAX_LEN;
    static const unsigned int NML_MAX_NR_OF_LEVELS;
    static const unsigned int NML_MAX_NR_OF_MENU_ITEMS;
    static const unsigned int MAX_NR_OF_WATCHES;
    static const unsigned int NML_MIN_NR_OF_ITEM_BYTES;
    static const unsigned int NML_NR_OF_HEADER_BYTES;
    static const unsigned short ROOT_OBJECT_ID;
    static const char *ObjectTypeString[];

    //-------------------- types ------------------------

    /// NML object types
    typedef enum {INVALID, MENU, PLAIN, TITLE, LIST} object_type_t;

    /// NML object id
    typedef unsigned short NewsObjectId_t;

    /// NML raw news object
    typedef struct
    {
        unsigned short nml_len;             ///< total length of NML data
        unsigned short extended_header_len; ///< length of ext. NML header
        unsigned char nml[4096];            ///< NML data
    } RawNewsObject_t;

    /// NML list or menu item
    typedef struct
    {
        std::string text;       ///< used for lists and menus
        unsigned short link_id; ///< only used for Menus
        bool link_id_available; ///< only used for Menus
    } Item_t;

    /// parsed NML news object
    typedef struct
    {
        NewsObjectId_t object_id;     ///< NML object id
        object_type_t object_type;    ///< NML object type
        bool static_flag;             ///< static flag
        unsigned char revision_index; ///< revision index
        std::string extended_header;  ///< extended header bytes (unparsed)
        std::string title;            ///< parsed title string
        std::vector<Item_t> item;     ///< items of list or menu object
    } News_t;

    //-------------------- lifetime --------------------
    NML();
    NML(const NML & prototype);
    ~NML();

    //-------------------- operators --------------------
    const NML & operator=(const NML & prototype);
    bool operator==(const NML & prototype) const;
    std::ostream & operator<<(std::ostream & os) const;

    //-------------------- inquiry methods --------------
    std::string Dump(void) const;

    /// check whether NML object is valid
    /// @retval false iff fake news object (error message)
    inline bool           isValid(void) const
    {
        return _valid;
    }

    /// check whether NML object is the root object
    /// @retval true if object is the root object
    inline bool           isRootObject(void) const
    {
        return (GetObjectId()==ROOT_OBJECT_ID);
    }

    /// check whether NML object is a menu
    /// @retval true if object is a menu object
    inline bool           isMenu(void) const
    {
        return (GetObjectType()==MENU);
    }

    /// check whether NML object is static
    /// @retval true if object is static
    inline bool           isStatic(void) const
    {
        return _news.static_flag;
    }

    /// NML object type
    /// @return object type
    inline object_type_t  GetObjectType(void) const
    {
        return _news.object_type;
    }

    /// NML object type string
    /// @return object type as string
    inline const char *GetObjectTypeString(void) const
    {
        if (_news.object_type<=LIST)
        {
            return NML::ObjectTypeString[_news.object_type];
        }
        else
        {
            return "illegal";
        }
    }

    /// NML object id
    /// @return object id
    inline NewsObjectId_t GetObjectId(void) const
    {
        return _news.object_id;
    }

    /// NML revision index
    /// @return revision index
    inline unsigned char  GetRevisionIndex(void) const
    {
        return _news.revision_index;
    }

    /// NML extended header
    /// @return extended header as string
    inline std::string    GetExtendedHeader(void) const
    {
        return _news.extended_header;
    }

    /// title of NML object
    /// @return title in UTF8-Coding
    inline std::string    GetTitle(void) const
    {
        return _news.title;
    }

    /// number of items contained in NML object
    /// @return number of items for lists and menus, 0 otherwise
    inline unsigned int   GetNrOfItems(void) const
    {
        return _news.item.size();
    }

    /// vector of items contained in NML object
    /// @return items for lists and menus, empty otherwise
    inline const std::vector<Item_t> & GetItems(void) const
    {
        return _news.item;
    }

    /// get specified item contained in NML object
    /// @return item text of i-th item
    inline std::string     GetItemText(unsigned int i) const
    {
        return (i<GetNrOfItems())?_news.item[i].text:"";
    }

    /// get specified link id contained in NML menu.
    /// Will only have a useful value if
    /// - the NML object is a menu
    /// - the index is not out of range
    /// @return link id of i-th menu item
    inline NewsObjectId_t  GetLinkId(unsigned int i) const
    {
        if (i>=GetNrOfItems()) return 0x0815;
        return _news.item[i].link_id;
    }

    /// check availability of a depending link id
    /// @param i  index of item in menu
    /// @retval true iff item's link id is available
    inline bool            isLinkIdAvailable(unsigned int i) const
    {
        return (i<GetNrOfItems()) ? _news.item[i].link_id_available : false;
    }

    //-------------------- modifier methods -------------

    /// set object id
    /// @param oid object id
    inline void SetObjectId(NewsObjectId_t oid)
    {
        _news.object_id = oid;
    }

    /// set i-th linked object id (in a menu) availability flag
    /// @param i index
    /// @param f new availability value
    inline void SetLinkAvailability(unsigned int i, bool f)
    {
        if (isMenu() && (i<GetNrOfItems()))
            _news.item[i].link_id_available = f;
    }

    void SetError(NewsObjectId_t oid, const char *title);
    void SetErrorDump(NewsObjectId_t oid,
                      const RawNewsObject_t & rno,
                      const char *error_msg);

private:
    bool _valid;
    News_t _news;
    NMLEscapeCodeHandler *_EscapeCodeHandler;
};

bool operator==(const NML::Item_t &, const NML::Item_t &);
std::string DumpRaw(const NML::RawNewsObject_t & rno);



// factory class for NML objects
class NMLFactory
{
public:
    static NMLFactory *Instance(void);
    static void ExitInstance(void);

    NML *CreateNML(const NML::RawNewsObject_t & rno,
                   const NMLEscapeCodeHandler *EscapeCodeHandler);
    NML *CreateError(NML::NewsObjectId_t oid,
                     const char *title);
    NML *CreateErrorDump(NML::NewsObjectId_t oid,
                         const NML::RawNewsObject_t & rno,
                         const char *error_msg);

private:
    static NMLFactory *_instance;
    NMLFactory();
    ~NMLFactory();
    NMLFactory & operator=(const NMLFactory &);
    NMLFactory(const NMLFactory &);
    unsigned char* getNextSection( const unsigned char*& p, unsigned short& plen, unsigned short& reslen );

};


#endif

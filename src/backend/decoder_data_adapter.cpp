/*
 *    Copyright (C) 2018
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <iostream>
#include <vector>
#include "decoder_data_adapter.h"
#include "MathHelper.h"
#include "dab-constants.h"
#include "tools.h"
#include <string>
#include <error.h>
#include <ctime>
#include <dirent.h>



#define STRING(num) STR(num)
#define STR(num) #num

// our_dabDataProcessor = make_unique<DecoderDataAdapter>(myProgrammeDataHandler, bitDataRate, DSCTy, dumpFileName);
//  `DecoderDataAdapter::DecoderDataAdapter(ProgrammeHandlerInterface&, short, short, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)'
DecoderDataAdapter::DecoderDataAdapter(ProgrammeHandlerInterface& mr, int16_t bitDataRate, int16_t DSCTy, const std::string& dumpFileName):
    bitDataRate(bitDataRate),     myInterface(mr)
        
    
{
   int32_t  appType = Global_appType;

   switch (DSCTy) {
	   default:
	      fprintf (stderr, "DSCTy %d not supported\n", DSCTy);
	      //my_dataHandler	= new virtual_dataHandler();
	      break;

	   case 5:
	      if (appType == 0x44a){
	       // our_dabJournalineProcessor	= new journaline_dataHandler();
          

         }
	      else
	      if (appType == 1500)
         {
	        // my_dataHandler = new adv_dataHandler (mr, dataBuffer, appType);
         }
         else
	        if (appType == 4)
	      {
            // my_dataHandler	= new tdc_dataHandler (mr, dataBuffer, appType);
         }
         else
	        if (appType == 0x61C0)
	      {
            // my_dataHandler	= new tdc_dataHandler (mr, dataBuffer, appType);
         }
         else
	        if (appType == 0xef0)
	      {
            // my_dataHandler	= new tdc_dataHandler (mr, dataBuffer, appType);
         }
	      else {
	         fprintf (stderr, "DSCTy 5 with appType %d not supported\n",appType);
	       //  my_dataHandler	= new virtual_dataHandler();
	      }
	      break;

	   case 44:
	     // my_dataHandler	= new journaline_dataHandler();
	      break;

	   case 59:
	     // my_dataHandler	= new ip_dataHandler (mr, dataBuffer);
	      break;

	   case 60:
         if (appType == 0){
	    //  my_dataHandler	= new motHandler (mr, backgroundFlag);
         }
         else
         if (appType == 7) {
   //  my_dataHandler	= new motHandler (mr, backgroundFlag);
   fprintf (stderr, "DSCTy 60 with appType %d not supported\n",appType);
         }
	      break;
	   
	}

}


// "DAB Data Thread" Aufruf von dab-data.cpp mit vorbehandelten Rohdaten, we have a final block of 24 bits
// In transmission mode I there are 12 FIBs and 4 CIFs per transmission frame of 96 ms. 
//void DecoderDataAdapter::addtoDataFrame(const std::vector<uint8_t>  outV)
void DecoderDataAdapter::addtoDatenFrame(uint8_t *v, int32_t fragmentSize, std::string dumpFileName , uint32_t DSCTy, uint32_t DGflag, uint32_t SId, uint32_t subChId,uint32_t appType) // so oft wie Datenstreams vorhanden ist
{
   if ((fragmentSize<=0)) {// Ausstieg, wenn keine Data da?!?
      fprintf(stderr, "zurück\n");
      return;

   }

   Global_DSCTy = DSCTy;
   Global_appType = appType;
   Global_DGFlag = DGflag;
   this->dumpFileName = dumpFileName;

 //  const size_t len = fragmentSize/8;
 const size_t len = 24 * bitDataRate /8; // nach bitdataraten Geschwindigkeit auswerten
std::vector<uint8_t>data(len*2);
std::vector<uint8_t>datatemp(len*2);
//auto          start       = std::chrono::system_clock::now(); // Capture
std::vector<uint8_t>Global_string80_90_a0_b0(6*len);


//Service *s = fichdl->fibProcessor.findServiceId(SId);

if ((DSCTy == 5) && (DGflag))	// no datagroups
  //handleTDCAsyncstream (v, 24 * bitDataRate);
  fprintf(stderr,"TDCA");
else
  handlePackets (v, 24 * bitDataRate);
 // fprintf(stderr,"Packets");

}

void	DecoderDataAdapter::handlePackets (uint8_t *dataL, uint16_t length) {
   uint8_t *data = dataL;
      while (true) {
   //	pLength is in bits
         size_t pLength = (getBits_2 (data, 0) + 1) * 24 * 8;
         if (length < pLength)	// be on the safe side
            return;
         handlePacket (data);

   //	prepare for the next round
         length -= pLength;
         if (length < 24) {
            return;
         }
         data = &(data [pLength]);
      }
   }

   void	DecoderDataAdapter::handlePacket (uint8_t *vec) {
      static int expected_cntidx = 0;
         uint8_t Length	= (getBits (vec, 0, 2) + 1) * 24;
         if (!check_CRC_bits (vec, Length * 8)) {
      	 //  fprintf (stderr, "crc fails %d\n", Length);
            return;
         }
      	// fprintf (stderr, "packet crc OK %d\n", Length);
      
      //	Continuity index:
         uint8_t cntIdx	= getBits (vec, 2, 2);
      //	First/Last flag:
         uint8_t flflg	= getBits (vec, 4, 2);
      //	Packet address
         uint16_t paddr	= getBits (vec, 6, 10);
      //	Useful data length
         uint8_t udlen	= getBits (vec, 17,7); 
      //	fprintf (stderr, "udlen = %d\n", udlen);
         if (udlen == 0)
            return;
      
/*          if (paddr != packetAddress)
            return; */
/*          if (cntIdx != expected_cntidx) {
      	   fprintf (stderr, "packet cntIdx %d expected %d address %d\n",
      	                                cntIdx, expected_cntidx, paddr);
            expected_cntidx = 0;
            return;
         } */
         expected_cntidx = (cntIdx + 1) % 4;
      
         switch (flflg) {
            case 2:  // First data group packet
               series. resize (udlen * 8);
               for (uint16_t i = 0; i < udlen * 8; i ++)
                  series [i] = vec [3 * 8 + i];
               assembling	= true;
               return;
      
            case 0:    // Intermediate data group packet
               if (assembling) {
                  int currentLength = series. size ();
                  if (currentLength + udlen * 8 > 4 * 8192) {
                     assembling = false;
      	            fprintf (stderr, "too large data group packet\n");
                     return;
                  }
                  series. resize (currentLength + udlen * 8);
                  for (int i = 0; i < udlen * 8; i ++) 
                     series [currentLength + i] = vec [3 * 8 + i];
               }
               return;
      
            case 1:  // Last data group packet
               if (assembling) {
                  int currentLength = series. size ();
                  if (currentLength + udlen * 8 > 4 * 8192) {
                     assembling = false;
      	            fprintf (stderr, "too large data group packet\n");
                     return;
                  }
                  series. resize (currentLength + udlen * 8);
                  for (int i = 0; i < udlen * 8; i ++)
                     series [currentLength + i] = vec [3 * 8 + i];
                  assembling = false;
      //
      //	Note, we are sending the UNPROCESSED mscdatagroup to the
      //	appropriate handler
          add_mscDatagroup (series);
          add_mscDatagroupTPEG(series);
                     
              //  fprintf(stderr, "add_mscDatagroup 1\n");
                  series. resize (0);
               }
               return;
      
               case 3: { // Single packet, mostly padding
                  if (Length > 3 * 8 + udlen * 8) {
                     series. resize (udlen * 8);
                     for (uint8_t i = 0; i < udlen * 8; i ++)
                        series [i] = vec [3 * 8 + i];
                    add_mscDatagroup (series);
                    //    fprintf(stderr, "\033[0;31madd_mscDatagroup  \033[0m\n");
                  }
                  series. resize (0);
                    }
               return;
      
               default:	// cannot happen
               fprintf(stderr, "\033[0;31mcannot happen \033[0m\n");
                  return;
            }
      }



      void	DecoderDataAdapter::add_mscDatagroup (std::vector<uint8_t> msc) {
         uint16_t	len	= msc. size ();
         uint8_t	*data	= (uint8_t *)(msc. data());
         uint8_t	*buffer	= (uint8_t *) alloca (len / 8 * sizeof (uint8_t));
         int32_t	res;
         uint16_t i;
         for ( i = 0; i < (len / 8); i ++)
               buffer [i] = getBits (data, 8 * i, 8);
         
         show_ascii_hex_row(buffer,len/8,0);
        write_dump_strings(buffer, len/8);

         }




void DecoderDataAdapter::show_ascii_hex_row(uint8_t *dataL, uint16_t length,size_t pos)
{
   uint8_t *Global_string80_90_a0_b0 = dataL;
   uint8_t ll,mm,oo,gg,pp,zz,ua,uaa,uaaa,kk,rt=0;
   uint16_t i, offset=0,o;
   FILE *fd ;
   int menuzaehler;
   
   //magiid=0x49871;
   //#define NEWS_SVC_MAGIC_ID	0x786245
   bool umlaut = false;
   start_png=false;
   png_zaehler=0;
   char us ;


   if ((length>44))
   {

            ua = Global_string80_90_a0_b0[0] ; // 1. ZeilenByte
            uaa = Global_string80_90_a0_b0[2] ; // 2. ZeilenByte
            uaaa = Global_string80_90_a0_b0[3] ; // 3. ZeilenByte



            for (i = pos+offset; i < length; i ++) {
               ll = Global_string80_90_a0_b0[i];  
               if ((ll==0xFE) && (start_titel==false)) rt=0x01; //start menu section
               if ((ll==0x06) and (start_titel==true)) { //ende menu section
                  start_titel=false;
               // fprintf(stderr,"%c",titel); 
                  for (int z =0;z<30;z++){
                     for (int zzi=0;zzi<40;zzi++)
                       titel[z][zzi] = 0x20;
                  }
               }
               if ((i+2<length))
               {
                  kk = Global_string80_90_a0_b0[i+1];
                  if ((ll==0x10) or (ll==0x0A)){
                   //   fprintf(stderr,"\n"); //Extended next line Preferred word break 
                  }
                  if ((ll==0xC3) and (kk==0xBC)) {
                    // fprintf(stderr,"\033[0;33mü\033[0m"); //Extended Data section ende
                     /* i++; */
                  }
                  if ((ll==0xC3) and (kk==0xA4)) 
                  {//fprintf(stderr,"\033[0;33mä\033[0m"); //Extended Data section ende
                  /*  i++; */}

                  if ((ll==0xC3) and (kk==0x9F)) 
                  {//fprintf(stderr,"\033[0;33mß\033[0m"); //Extended Data section ende
                  /*   i++; */}
                  if ((ll==0xC3) and (kk==0xB6)) 
                  {//fprintf(stderr,"\033[0;33mö\033[0m"); //Extended Data section ende
                  /*  i++; */
                  }
               }
               if ((i+80<=length))
               {
                  kk = Global_string80_90_a0_b0[i+1] & 0xf;
                  mm = Global_string80_90_a0_b0[i+2]; // menu eins
                  oo = Global_string80_90_a0_b0[i+3]; // länge
                  gg = Global_string80_90_a0_b0[i+4]; // ist wert ein buchstabe
                  if (((ll==0x1A) || (ll==0x1B))  and (!start_data)) { 
                     start_data=true;
                  // we take the length of the datasection and add it
                  // to the current position in order to ignore it
                     data_len = Global_string80_90_a0_b0[i+1] + 1;
                     //fprintf(stderr,"\n#### start data #### len=%i\n",data_len);
                  }
                  if ((ll==0xF0) and start_data) { 
                        //fprintf(stderr,"\n#### ende data ####\n)");
                        start_data=false;
                  }
                  
                  if ((ll==0x04) and (mm==0x01) and (rt==0x01) and (ua==0x74) and (uaa==0x00) and (uaaa==0x00)) { // such start index für ersten Menueintrag
                           //fprintf(stderr,"start Titel\n");
                           start_titel=true;
                           titel_zaehler=0;
                           titel_header_zaehler=0;
                           rt=0;
         // ##################################################   
                           i=15;     // ab Pos 15 geht Menu los
                           zz=0;  
                           for (menuzaehler=0; menuzaehler<30; menuzaehler++){    
                                    ll = Global_string80_90_a0_b0[i]; // Zähler
                                    oo = Global_string80_90_a0_b0[i+1]; // länge
                                    i=i+2;

                                       if ((ll==06) and (oo==02)) { // Menu ExitCode und darstellung der Menüs
 // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++                                
                                          int tt=0;
                                          while  (tt<menuzaehler){
                                             int zz =0 ;
                                             
                                             while (titel[tt][zz]!=0x20){
                                                fprintf(stderr,"\033[0;34m%c\033[0m", titel[tt][zz++]); //print menu
                                                
                                             }
                                             tt++;
                                             fprintf(stderr,"\033[0;34m \033[0m"); //print space
                                          }
                                             goto weiter; // hier weiter mit
                                          }
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                    for (o = i; o < i+oo; o ++) {
                                       if (o+3>length) 
                                          goto weiter;
                                       ll = Global_string80_90_a0_b0[o];
                                       //********************************************************* */
                                       if ((o+2<length))
                                       {
                                          kk = Global_string80_90_a0_b0[o+1];
                                          if ((ll==0x10) or (ll==0x0A)){
                                           //   fprintf(stderr,"\n"); //Extended next line Preferred word break 
                                          }
                                          if ((ll==0xC3) and (kk==0xBC)) {
                                           //  fprintf(stderr,"\033[0;34mü\033[0m"); //Extended Data section ende
                                             umlaut = true;
                                             titel[menuzaehler][zz++]=0x75;
                                             titel[menuzaehler][zz++]=0x65;
                                             zz++;
                                          }
                                          if ((ll==0xC3) and (kk==0xA4)) 
                                          {//fprintf(stderr,"\033[0;34mä\033[0m"); //Extended Data section ende
                                             umlaut = true;   
                                             titel[menuzaehler][zz++]=0x61;
                                             titel[menuzaehler][zz++]=0x65;
                                             
                                          }

                                          if ((ll==0xC3) and (kk==0x9F)) 
                                          {//fprintf(stderr,"\033[0;34mß\033[0m"); //Extended Data section ende
                                             umlaut = true;
                                             titel[menuzaehler][zz++]=0x73;
                                             titel[menuzaehler][zz++]=0x73;
                                             
                                          }
                                          if ((ll==0xC3) and (kk==0xB6)) 
                                          {//fprintf(stderr,"\033[0;34mö\033[0m"); //Extended Data section ende
                                             umlaut = true;
                                             titel[menuzaehler][zz++]=0x6F;
                                             titel[menuzaehler][zz++]=0x65;
                                             
                                          }
                                       }

                                       if (((ll>31) && (ll<128)))
                                          {
                                           //  fprintf(stderr,"\033[0;34m%c\033[0m", toascii(ll)); 
                                             titel[menuzaehler][zz++]=ll;
                                          }
                                         }
                                    i=o;
                                    //fprintf(stderr," ");
                                    titel[menuzaehler][zz]=0x20;
                                    zz=0;

                           }
                        }

       if (ll>0x7F) continue;
/*                      if (((ll>31) && (ll<128)))
                     fprintf(stderr,"\033[0;33m%c\033[0m", toascii(ll));  // ausgabe in ascii
                  else
                     fprintf(stderr,"  "); */
               }
            }
                     // ##################################################
 weiter: 
          //  fprintf(stderr,"\n"); 
            for (i = pos/* +offset */; i < length; i ++) {
               ll = Global_string80_90_a0_b0[i];   
               if (i+5<length)
               {
                  oo = Global_string80_90_a0_b0[i+1]; 
                  gg = Global_string80_90_a0_b0[i+2];
                  mm = Global_string80_90_a0_b0[i+3];
                  pp = Global_string80_90_a0_b0[i+4];
                  zz = Global_string80_90_a0_b0[i+5];
               }  

               
               if ((ll==0x40) and (gg==0xFF) and (mm==0x0F) and (pp==0x0F)){ //tmc
                  //fprintf(stderr,"start TPEG TMC");
                  start_name_TMC=true;      
                  firstSegment_name_TMC=i+1;
#if 0                  
                  write_hex(Global_string80_90_a0_b0, length);
#endif
               }
               if (ll==0x40) { // doch kein png, sondern TMC
                  //fprintf(stderr,"start png Name");
                  start_name_png=true;
                  firstSegment_name_png=i+1;
               }
               if ((ll==0x00) and (oo==0x0F) and (zz==0x08) and (start_name_TMC==true))
                  { //TMC Text
                  //fprintf(stderr,"start png Name");
                  start_name_TMC=false;
                  lastSegment_name_TMC=i;
                  for (size_t uu=i; uu<pp+i;uu++)
                   {
                     us = toascii( Global_string80_90_a0_b0[uu]);
                    // fprintf(stderr,"\033[0;35m%c\033[0m",us);
                   }
                  
               }

          
               //if (ll>0x7F) continue;
               // if (ll==0x01) fprintf(stderr,"\033[0;94m[[\033[0m"); //Preferred word break 
               // if (ll==0x02) fprintf(stderr,"\033[0;94m]]\033[0m"); //Extended Highlight start 
               // if (ll==0x03) fprintf(stderr,"\033[0;94m((\033[0m"); //Extended Highlight  end  
               // if (ll==0x04) fprintf(stderr,"\033[0;94m))\033[0m"); //End of introductory section 
               // if (ll==0x05) fprintf(stderr,"\033[0;94m//\033[0m"); //End of introductory section 
               // if (ll==0x06) fprintf(stderr,"\033[0;94m//\033[0m"); //End of menu section 
               // if (ll==0x11) fprintf(stderr,"\033[0;94m#\033[0m"); //Preferred word break 
               // if (ll==0x12) fprintf(stderr,"\033[0;94m*<\033[0m"); //Extended Highlight start 
               // if (ll==0x13) fprintf(stderr,"\033[0;94m>*\033[0m"); //Extended Highlight  end  
               // if (ll==0x14) fprintf(stderr,"\033[0;94m@\033[0m"); //End of introductory section 
               // if (ll==0x1C) fprintf(stderr,"\033[0;94m-<\033[0m"); //Extended code begin 
               // if (ll==0x1D) fprintf(stderr,"\033[0;94m>-\033[0m"); //Extended code end  
               // if (ll==0x1A) fprintf(stderr,"\033[0;94m+<\033[0m"); //Extended Data section start
               // if (ll==0x1B) fprintf(stderr,"\033[0;94m>+\033[0m"); //Extended Data section ende 

               if ((ll==0x8D) && (start_name_png==true)){ //png
                  //fprintf(stderr,"start png Name");
                  start_name_png=false;
                  lastSegment_name_png=i;
                  if (((lastSegment_name_png-firstSegment_name_png)>=10) && ((lastSegment_name_png-firstSegment_name_png)<=26))
                     {
                     if ((Global_string80_90_a0_b0[lastSegment_name_png-3]==0x70) && (Global_string80_90_a0_b0[lastSegment_name_png-2]==0x6E) && (Global_string80_90_a0_b0[lastSegment_name_png-1]==0x67) )
                        {
                           
                           for (int uu=firstSegment_name_png; uu<lastSegment_name_png;uu++)
                              {
                                 us = toascii( Global_string80_90_a0_b0[uu]);
                                // fprintf(stderr,"\033[0;35m%c\033[0m",us);  // ascii ausgabe
                              }
                        }
                     }

               }
          

               if (((ll==0x89) && (oo==0x50) && (gg==0x4E)&& (mm==0x47)&& (pp==0x0D)&& (zz==0x0A)) && (start_png==false)){ //png
                    // fprintf(stderr,"\033[0;33mstart png\033[0m ");
                     start_png=true;
                     png_zaehler=0;
               } 

               if (start_png==true)
                  { png[png_zaehler++]=ll;
                  }
               if (((ll==0x4E) &&(oo==0x44) && (gg==0xAE) && (mm==0x42)&& (pp==0x60)& (zz==0x82))  && (start_png==true )){ //png
                  
                  start_png=false;
                  /*          png[png_zaehler++]=0x49;
                           png[png_zaehler++]=0x45;
                  png[png_zaehler++]=0x4E; */
                  png[png_zaehler++]=0x44;
                  png[png_zaehler++]=0xAE;
                  png[png_zaehler++]=0x42;
                  png[png_zaehler++]=0x60;
                  png[png_zaehler++]=0x82;

                     // Open a dump file (XEPGxpert) if the user defined it
                  if (!this->dumpFileName.empty()) {
                        std::string num_cstr(STRING(png_zaehler));

                        std::string FileName = "/tmp/" + this->dumpFileName + std::to_string( png_zaehler) + /* " " +  std::to_string( time(0)) */ +  ".png";
                       
                        bool erg = write_only_on_png(DAB_TPEG_PNG_STR, occurrencesPNG, FileName); // prüfe ob schon einmal das png geschrieben wurde
                       if (erg==true) {
                        if ((png_zaehler>50) && (png[0]==0x89) && (png[1]==0x50)){   // png = 89 50 4e 47 0d 0a 1a 0a  
                           fd = fopen((FileName.c_str()), "wb+");
                     
                           if ((fd) ){
                            //  size_t arraySize = sizeof(png) / sizeof(png[0]);
                             // Array in die Datei schreiben

                             float elementsWritten = std::fwrite(&png[0],sizeof(short), png_zaehler-2 ,fd);
                             fprintf (stderr,"\033[0;32mschreibe %s\033[0;m",FileName.c_str());
                              fclose(fd);
                              if (elementsWritten != png_zaehler-1) {
                                 fprintf (stderr,"\033[0;32mFehler beim Schreiben png!\033[0;m");
                              }
                           }
                           else {
                              if (fd) fclose(fd);
                              
                           }
                        }
                     }
                           png_zaehler=0;
                  }
               } 
            }
         //   fprintf(stderr,"\n\n"); 

         }
      }

      // wenn location 2 x vorkommt, dann stelle location dar
bool DecoderDataAdapter::write_only_on_png(std::vector<std::string>& arraySTR, std::unordered_map<std::string, int>& countMapSTR, std::string dataSTR) {
   //fprintf(stderr,"%04x, ", value);
    countMapSTR[dataSTR]++; // Zähler für die Zahl erhöhen
    if (countMapSTR[dataSTR] == 1) {
        arraySTR.push_back(dataSTR); // Nur beim ersten Auftreten speichern
        return true;
    } else if (countMapSTR[dataSTR] > 2) {
        //std::cout << "Zahl erscheint zum dritten Mal: " << value << std::endl;
        //tpeg_tmc_sqlite (location, event); // erst anzeigen wenn das dritte mal erscheint
        return false;
    }
    return false;
}


      void DecoderDataAdapter::write_dump_strings(uint8_t *temp_dump,uint16_t framebytes)
{
      FILE * DumpmessageDumper;
      std::string FileName = "/tmp/" + this->dumpFileName + std::to_string(framebytes) + ".edt";
      int umbruchzaehler =0;  
      bool umlaut = false;
// test test test, erst bei x Umbrüchen in Datei schreiben

                           for (size_t i = 0; i < framebytes; i++) {
                              if (((temp_dump[i]) /* & 0x80 */) == 0x80)  { // trenne bei 0x8x den string
                                 umbruchzaehler++;
                                 if (umbruchzaehler>12)  // umbruchzähler
	                                 goto umbruchweiter;

	                           }
                           }
                           goto ausstieg;
// test test test
umbruchweiter:


      DumpmessageDumper = fopen((FileName.c_str()), "wb+");

                           if ((DumpmessageDumper) ){
                           for (size_t i = 0; i < framebytes; i++) {
                              if (((temp_dump[i]) /* & 0x80 */) == 0x80)  { // trenne bei 0x8x den string
	                                 fprintf(DumpmessageDumper, "\n"); // Hexadezimalwerte in Datei schreiben
                                 i++;
	                           }
                              else {
                                    fprintf(DumpmessageDumper, "%02x ", temp_dump[i]); // Hexadezimalwerte formatiert in Datei schreiben
                                // fprintf(DumpmessageDumper, "%c ", toascii(temp_dump[i])); // Hexadezimalwerte formatiert in Datei schreiben
                              }
                              //fprintf(TPEGmessageDumper, "\n"); // Hexadezimalwerte in Datei schreiben
                           }

                            fprintf(DumpmessageDumper, "\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"); // Hexadezimalwerte formatiert in Datei schreiben
                           for (size_t i = 0; i < framebytes-1; i++) {
                              if (((temp_dump[i]) /* & 0x80 */) == 0x80)  { // trenne bei 0x8x den string
	                              fprintf(DumpmessageDumper, "\n"); // Hexadezimalwerte in Datei schreiben
                                 i++;
	                           }
                              else {
                                    umlaut = false;     
                                    if ((temp_dump[i]==0x10) or (temp_dump[i]==0x0A)){
                                       fprintf(DumpmessageDumper, "%c", toascii(0x0a));
                                       
                                    }
                                    if ((temp_dump[i]==0xC3) and (temp_dump[i+1]==0xBC)) {
                                       fprintf(DumpmessageDumper, "%s", "ue");
                                       i++;
                                       umlaut = true;
                                    }
                                    if ((temp_dump[i]==0xC3) and (temp_dump[i+1]==0xA4)) 
                                    {fprintf(DumpmessageDumper, "%s", "ae");
                                     i++;
                                    umlaut = true;}

                                    if ((temp_dump[i]==0xC3) and (temp_dump[i+1]==0x9F)) 
                                    {fprintf(DumpmessageDumper, "%s", "ss");
                                      i++;
                                    umlaut = true;}
                                    if ((temp_dump[i]==0xC3) and (temp_dump[i+1]==0xB6)) 
                                    {fprintf(DumpmessageDumper, "%s", "oe");
                                     i++;
                                     umlaut = true;
                                    }

                                    if ((umlaut == false) && ((temp_dump[i]>31) && (temp_dump[i]<128)))
                                       fprintf(DumpmessageDumper, "%c", toascii(temp_dump[i])); // Hexadezimalwerte formatiert in Datei schreiben
                              }
                              //fprintf(TPEGmessageDumper, "\n"); // Hexadezimalwerte in Datei schreiben
                           }


                           fprintf(stderr,"\033[0;32mschreibe %s\033[0;m\n",FileName.c_str());
                           fclose(DumpmessageDumper);
                              }

                           else
                              if (DumpmessageDumper) fclose(DumpmessageDumper);
                     
      ausstieg:
       fprintf(stderr, ""); 
  }

      void DecoderDataAdapter::write_tpeg_strings(std::vector<uint8_t> temp_tpeg,uint16_t framebytes)
{
      FILE * TPEGmessageDumper;
      std::string FileName = "/tmp/" + this->dumpFileName + std::to_string(framebytes) + ".tpg";
                        
                           TPEGmessageDumper = fopen((FileName.c_str()), "wb+");

                           if ((TPEGmessageDumper) ){
                           for (size_t i = 0; i < framebytes-1; i++) {
                              if (((temp_tpeg[i]) == 0x00) && ((temp_tpeg[i+1]) == 0x00)) { // trenne bei 0x0000 den string
	                              fprintf(TPEGmessageDumper, "\n\n"); // Hexadezimalwerte in Datei schreiben
                                 i++;
	                           }
                              else {
                                 fprintf(TPEGmessageDumper, "%02x ", temp_tpeg[i]); // Hexadezimalwerte formatiert in Datei schreiben
                              }
                              //fprintf(TPEGmessageDumper, "\n"); // Hexadezimalwerte in Datei schreiben
                           }
                           fprintf(stderr,"\033[0;32mschreibe %s\033[0;m\n",FileName.c_str());
                           fclose(TPEGmessageDumper);
                              }

                           else
                              if (TPEGmessageDumper) fclose(TPEGmessageDumper);
                     
      

  }
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void DecoderDataAdapter::write_hex(uint8_t *dataL, uint16_t length)
{
      FILE * TPEGframeDumper;
      dataL++; // überspring ein byte
      dataL++;  // überspring ein byte
      std::string FileName = "/tmp/" + this->dumpFileName + std::to_string(length) + ".txt";
                     //   fprintf(stderr,"%s",FileName);
                           TPEGframeDumper = fopen((FileName.c_str()), "wb+");

                           if ((TPEGframeDumper) ){
                            //  size_t arraySize = sizeof(png) / sizeof(png[0]);
                             // Array in die Datei schreiben
                             std::fwrite(dataL,sizeof(short), length/2-1  ,TPEGframeDumper);
                    
                              fclose(TPEGframeDumper);
/*                               if (elementsWritten != length) {
                                 fprintf (stderr,"Fehler beim Schreiben des Arrays!");
                              } */
                           }
                           else
                              if (TPEGframeDumper) fclose(TPEGframeDumper);
                     
      

  }
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define	swap(a)	(((a) << 8) | ((a) >> 8))

//---------------------------------------------------------------------------
uint16_t usCalculCRC (uint8_t *buf, int lg) {
uint16_t crc;
int	count;
	crc = 0xFFFF;
	for (count = 0; count < lg; count++) {
	   crc = (uint16_t) (swap (crc) ^ (uint16_t)buf [count]);
	   crc ^= ((uint8_t)crc) >> 4;
	   crc = (uint16_t)
	         (crc ^ (swap((uint8_t)(crc)) << 4) ^ ((uint8_t)(crc) << 5));
	}
	return ((uint16_t)(crc ^ 0xFFFF));
}


void	DecoderDataAdapter::add_mscDatagroupTPEG (std::vector<uint8_t> m) { // hier kommt DAB MSC Data Group an
   // Data Group Header, Session Header, Data Group Data
int32_t	offset	= 0;
uint8_t	*data	= (uint8_t *)(m. data());
int32_t	size	= m. size();
int16_t	i;

/* data++;   // ein byte überspringen
data++; // ein byte überspringen */

//	we maintain offsets in bits, the "m" array has one bit per byte
	while (offset < size) {
	   while (offset + 54 < size) {
	      if (getBits (data, offset, 16) == 0xFF0F) {  // sync word für tpeg im Transportframe, data[0][1]
   //      if ((getBits (data, offset, 16) == 0xFF0F) || (getBits (data, offset, 16) == 0x00FF)  || (getBits (data, offset, 16) == 0x0000)) {  // sync word für tpeg im Transportframe
	         break;
	      }
	      else
            offset += 8;
	   }
	   if (offset + 48 >= size)
	      return;
//	fprintf (stderr, "we have a syncword  0xff0f\n");

//	we have a syncword
	   uint16_t syncword	= getBits (data, offset,      16);
	   int16_t length	= getBits (data, offset + 16, 16); // im Transportframe    ((TPEGstring.data[index + 2] << 8) + (TPEGstring.data[index + 3]))
	   uint16_t crc		= getBits (data, offset + 32, 16); // im Transportframe ((TPEGstring.data[index + 4] << 8) + (TPEGstring.data[index + 5]))

	   (void)crc;
	   uint8_t frametypeIndicator = getBits (data, offset + 48,  8); // im Transportframe
    //  uint8_t frametypeIndicator = getBits_8 (data, offset + 48); // im Transportframe
	   if ((length < 0) || (length >= (size - offset) / 8))
	      return;		// garbage
//
//	OK, prepare to check the crc
	   uint8_t checkVector [18];
//
//	first the syncword and the length
	   for (i = 0; i < 4; i ++)
	      checkVector [i] = getBits (data, offset + i * 8, 8);
//
//	we skip the crc in the incoming data and take the frametype
	   checkVector [4] = getBits (data, offset + 6 * 8, 8);

	   int size = length < 11 ? length : 11;
	   for (i = 0; i < size; i ++)
	      checkVector [5 + i] = getBits (data,  offset + 7 * 8 + i * 8, 8);
	   checkVector [5 + size]	= getBits (data, offset + 4 * 8, 8);
	   checkVector [5 + size + 1]	= getBits (data, offset + 5 * 8, 8);
	   if (!check_crc_bytes (checkVector, 5 + size)) {
	      fprintf (stderr, "crc failed\n");
	      return;
	   }

//	   fprintf (stderr, "frametype %d\n", frametypeIndicator);
	   if (frametypeIndicator == 0) 
	      offset = handleFrame_type_0 (data, offset + 7 * 8, length); // -> Serviceframe Text
	   else
	   if (frametypeIndicator == 1) {
	      if (8 * length + offset + 7 * 8 < (int)m. size ())
	         offset = handleFrame_type_1 (data, offset + 7 * 8, length); // -> Serviceframe BIN
               else
	         offset  += length;
	   }
	   else
	      return;	// failure
	}
}
 
int32_t	DecoderDataAdapter::handleFrame_type_0 (uint8_t *data, int32_t offset, int32_t length) { // -> Serviceframe Text
//int16_t noS	= getBits (data, offset, 8);
uint8_t *buffer	= (uint8_t *) alloca (length * sizeof (uint8_t));
	
	for (uint16_t i = 0; i < length; i ++)
	   buffer [i] = getBits (data, offset + i * 8, 8);                // -> Service Component Frame in TEXT in Bytes
	if (!check_crc_bytes (buffer, length - 2))
	   fprintf (stderr, "crc ook hier fout\n");
      uint8_t nrServices = buffer[0];
      uint8_t SIDa = buffer[1];
      uint8_t SIDb = buffer[2];
      uint8_t SIDc = buffer[3];
      uint8_t frameCRC = buffer[4];
#if 0
	   fprintf (stderr, "nrServices %d, SID-A %d SID-B %d SID-C %d\n",nrServices, SIDa, SIDb, SIDc);
#endif
      uint8_t range_len = buffer[0];

      //for (const auto i : range_len())

   for (int g=0;g<length;g++)
      Global_string80_90_a0_b0[g] = buffer[g];

   handle_TPEG_Message0(buffer,0,length);
	/////bytesOut (0, length);
	return offset + length * 8;
}


// Terminology for both:               Binary      XML versions:
//   Road Traffic Messages:           TPEG-RTM    tpeg-rtmML
//   Public Transport Information:    TPEG-PTI    tpeg-ptiML
//   Location Referencing:            TPEG-LOC    tpeg-locML
//   Parking Information:             TPEG-PKI
//   Traffic Event Compact:           TPEG-TEC
// parsing of standard component header
int32_t	DecoderDataAdapter::handleFrame_type_1 (uint8_t *data, int32_t offset, int32_t length) {  // -> Serviceframe BIN
      uint8_t	*buffer = (uint8_t *) alloca (length * sizeof (uint8_t));
      int	lOffset;
      int	llengths = length - 4;
      uint8_t SIDa = getBits (data, offset,      8); // data [0]  ,    Service SID-A
      uint8_t SIDb = getBits (data, offset + 8,  8); // dataBuffer [1]  ,    Service SID-B
      uint8_t SIDc = getBits (data, offset + 16, 8); // dataBuffer [2]  ,    Service SID-C
      uint8_t encID = getBits (data, offset + 24, 8); // dataBuffer [3]  ,    Encyption Indicator
      uint8_t textlen = getBits (data, offset + 48, 8); // dataBuffer [3]  ,    Encyption Indicator
     // fprintf(stderr,"%x",textlen);
#if 0
	fprintf (stderr, " frametype 1  (length %02x) mit offset=%02x SID-A=%02x SID-B=%02x SID-C=%02x\n", length, offset, SIDa, SIDb, SIDc);
	fprintf (stderr, "encryption=%d\n", encID);
#endif
      


	for (uint16_t i = 0; i < length; i ++)    // Serviceframe BIN
	   buffer [i] = getBits (data, offset + i * 8, 8);             // Service Component Frame in Bits
	
   if (getBits (data, offset + 24, 8) == 0) {	// no encryption  dataBuffer [3]  ,    Encyption Indicator
	   lOffset	= offset + 4 * 8;
      do
      { // parsing of standard component header
	      int compInd	= getBits (data, lOffset, 8);	// Serviceframe ServiceComponent ID, 0x0d = index
	      int flength	= getBits (data, lOffset + 8, 16); // Serviceframe ServiceComponent Field length, 0x0e & 0x0f=länge
	      int crc		= getBits (data, lOffset + 3 * 8, 8); // Serviceframe ServiceComponent Header CRC, 0x0e = crc
// #if 0
	      // fprintf (stderr, "segment %d, length %d\n", compInd, flength);
	      // for (int i = ii; i < flength; i ++)
	      //   fprintf (stderr, "%c", buffer [i]);                 // Service Component Frame in CHAR
         handle_TPEG_Message1(data,textlen,flength);
	      // fprintf (stderr, "\n");
// #endif
	      lOffset	+= (flength + 5) * 8;
         llengths -= flength + 5;
	   } while (llengths > 10);
	}
	/////bytesOut (1, length);
	return offset + length * 8;
}


void DecoderDataAdapter::handle_TPEG_Message0 (uint8_t *data, int32_t offset, int32_t length)
{

fprintf(stderr,"\033[0;94m ++++++++++++++++++ message 0 +++++++++++++++++++ \033[0;m\n");
   
}


//  TPEG-Message
//  - Message Management Container
//  - Application Container (e.g. TPEG-RTM)
//       -& ACC, ACT, CON,...
//  - Location Referencing Container
//    -* Default Language Code
//    -* Location Cordiantes   
//          -*# Location Type 
//          -*# Node Type List
//          -*# WGS 84
//          -*# Descriptor 
//          -*# Direction Type
//    -* Additional Location Description
void DecoderDataAdapter::handle_TPEG_Message1 (uint8_t *m, int32_t doffset, int32_t dlength)
{
   std::vector<uint8_t>tpeg_string(dlength);
   uint8_t *data = m;
       // Convert 8 bits (stored in one uint8) into one uint8               jjjjjjjj  - >    i
    for (int32_t i = 0; i < dlength; i ++) { // aus 8 bits mache ein byte , "11001011" - >  "CB"
        tpeg_string[i]=0;
        for (int j = 0; j < 8; j ++) {
            tpeg_string[i] <<= 1;
            tpeg_string[i] |= m[8 * i + j] & 01; // *v -> tpeg_string[i]
            
         //fprintf(stderr,"DecoderDataAdapter::addtoFrame i=%x,  j=%x, Data[]=%x, xchangeData[]=%x\n", i,j,data[i], v[i]);
        }
     //  fprintf(stderr,"\033[0;94m%02x\033[0;",tpeg_string[i]);
    }

   
   uint8_t drueber = 2*8; // überlese erste 2 bytes, dann kommt 0xff0f
   //uint8_t oo,gg,pp,zz,ua,uaa,uaaa,kk,rt=0;
   int32_t	offset	= getBits (data, 0x0d*8, 8); // start data
   int32_t	size	= dlength*8;
   //uint8_t u=0;
   size_t i=0;
   uint8_t k=0;
   uint16_t syncword = getBits (data, drueber+0*8, 16); // syncword 0xff0f 0x00
   uint16_t framebytes = getBits (data,drueber+ 2*8, 16); // Bytes im frame 0x02
   uint16_t crc = getBits (data,drueber+ 4*8, 16); // crc                  0x04
   uint8_t frame = getBits (data, drueber+6*8, 8); // frame                0x06
   uint8_t sid1 = getBits (data, drueber+7*8, 8); // sid1                  0x07
   uint8_t sid2 = getBits (data,drueber+ 8*8, 8); // sid2                  0x08
   uint8_t sid3 = getBits (data, drueber+9*8, 8); // sid3                  0x09
   uint8_t encID = getBits (data, drueber+0x0a*8, 8); // encID             0x0a
   uint8_t fieldlength = getBits (data, drueber+0x0d*8, 8); // fieldlength  0x0d
   uint8_t messagecount = getBits (data, drueber+0x10*8, 8); // message count 0x10
   uint16_t snicomponentid = getBits (data, drueber+0x11*8, 16); // snicomponentid  0x13
   uint8_t snicomponentlength = getBits (data, drueber+0x13*8, 8); // snicomponentidlength  0x13
   uint16_t headercrc = getBits (data, drueber+0x0e*8, 16); //  header crc   0x0e
   uint8_t textlen = getBits (data, drueber+0x27*8, 8); // startdata       0x27
   uint8_t ServiceName = getBits (data, drueber+0x28*8, 8); // ServiceName       0x28
   uint8_t ServiceDescription = getBits (data,drueber+0x28*8+(ServiceName+1)*8 , 8); // ServiceDescription 
   uint8_t scid = getBits (data, drueber+fieldlength*8, 8); // scid          0x6a
   uint16_t datacomps = getBits (data, drueber+fieldlength*8+0x01*8, 16); // startcomps 0x6b ??
   uint16_t dataCRC = getBits (data, drueber+fieldlength*8+0x03*8, 16); // data crc 0x6d ??

	fprintf (stderr, " frametype 1  (syncword %02x) (length %02x) mit offset=%02x SID-A=%02x SID-B=%02x SID-C=%02x, ", syncword, framebytes, drueber, sid1, sid2, sid3);
	fprintf (stderr, "encryption=%d, ", encID);
   fprintf (stderr, "crc=%02x, ", crc);
   fprintf (stderr, "frame=%02x, ", frame);
   fprintf (stderr, "fieldlength=%02x, ", fieldlength);
   fprintf (stderr, "textlen=%02x \n", textlen);
       for (k=0x29; k<=0x29+ ServiceName+1;k++) { 
      uint8_t textbyte = getBits (data, k*8, 8); // ServiceName text bytes
      fprintf(stderr,"\033[0;94m%c\033[0;m",textbyte);
    }

    fprintf(stderr," ");

      for (k=0x29+ServiceName+3; k<=0x29+ServiceName+3  +ServiceDescription;k++) { 
      uint8_t textbyte = getBits (data, k*8, 8); // ServiceDescription text bytes
      fprintf(stderr,"\033[0;95m%c\033[0;m",textbyte);
    }
   fprintf (stderr, ", scid=%02x, ", scid);
   fprintf (stderr, "datacomps=%02x, ", datacomps);
   fprintf (stderr, "headercrc=%04x, ", headercrc);
   fprintf (stderr, "dataCRC=%04x\n", dataCRC);

  //	split string with 0x0000 or 0x000f or 0x 00ff

  while (offset < size) {
      while (offset + 16 < size) {
	      if ((getBits (data, offset, 16) == 0x0000) || (getBits (data, offset, 16) == 0x000F)|| (getBits (data, offset, 16) == 0x00FF)) {
	         break;
	      }
	      else {
            temptpeg[i]=  tpeg_string[offset/8];
        //    fprintf(stderr,"\033[0;95m%02x \033[0;m",temptpeg[i] ); // ausgabe hex
            offset += 8;
            i++;
         }

      }
     // fprintf(stderr,"\n");  //  ausgabe hex trenner
            
	   if (offset + 16 >= size){
         //fprintf(stderr,"\n");
         if ((i>1000) & (i<3700)) // keine kleine Dateien erzeugen oder sehr grosse Dateien, damit tmp nicht voll läuft
            write_tpeg_strings(tpeg_string, i);
         handle_tpeg_tmc(tpeg_string, i);
         return;
      }
//	fprintf (stderr, "1. syncword 0x0000 oder 0x000f gefunden\n");
      offset += 16;
      }
//fprintf(stderr,"\n");


}


//	The component header CRC is two bytes long,
//	and based on the ITU-T polynomial x^16 + x*12 + x^5 + 1.
//	The component header CRC is calculated from the service component
//	identifier, the field length and the first 13 bytes of the
//	component data. In the case of component data shorter
//	than 13 bytes, the component identifier, the field
//	length and all component data shall be taken into account.
bool	DecoderDataAdapter::serviceComponentFrameheaderCRC (uint8_t *data, int16_t offset, int16_t maxL) { // Serviceframe ServiceComponent Header CRC
uint8_t testVector [18];
int16_t	length	= getBits (data, offset + 8, 16);
int16_t	size	= length < 13 ? length : 13;
uint16_t	crc;
	(void)maxL;
	if (length < 0)
	   return false;		// assumed garbage
	crc	= getBits (data, offset + 24, 16); 	// the crc
	testVector [0]	= getBits (data, offset + 0,  8);
	testVector [1]	= getBits (data, offset + 8,  8);
	testVector [2]	= getBits (data, offset + 16, 8);
	for (uint16_t i = 0; i < size; i ++) 
	   testVector [3 + i] = getBits (data, offset + 40 + i * 8, 8);

	return usCalculCRC (testVector, 3 + size) == crc;
}

// wenn location 2 x vorkommt, dann stelle location dar
void DecoderDataAdapter::addOrReportDuplicate(std::vector<uint16_t>& array, std::unordered_map<uint16_t, int>& countMap, uint16_t location, uint16_t event) {
   //fprintf(stderr,"%04x, ", value);
    countMap[location]++; // Zähler für die Zahl erhöhen
    if (countMap[location] == 1) {
        array.push_back(location); // Nur beim ersten Auftreten speichern
    } else if (countMap[location] > 2) {
        //std::cout << "Zahl erscheint zum dritten Mal: " << value << std::endl;
        tpeg_tmc_sqlite (location, event); // erst anzeigen wenn das dritte mal erscheint
    }
}



/*      gGroupCode = ((lo_block2 And &HF8) >> 3) 'Fertig; GroupTypeCode
        gGroupB0 = ((lo_block2 And &H8) >> 3) 'Fertig; B0
        gGroupTP = ((lo_block2 And &H4) >> 2) 'Fertig; TP
        gGroupTMCgetType = ((hi_block2 And &H18) >> 3) 'X4,X3 Fertig; Usermeldung/Group=0, Usermeldung/Single=1, Tuning/Group=2, Tuning/Single=3 
        gGroupTMCgetDuration = (hi_block2 And &H7) 'X2,X1,X0 Fertig; Dauer * 5 Minuten
        gGroupTMCgetCI = (hi_block2 And &H7) 'X2,X1,X0 Fertig; Dauer * 5 Minuten
        gGroupTMCgetSGI = ((lo_block3 And &H40) >> 6) ' Y15 Fertig; 0=keine Umleitung , 1=Umleitung
        gGroupTMCgetGSI = ((lo_block3 And &H30) >> 4) ' Y14 Fertig;0=positive Richtung 1=negative Richtung
        gGroupTMCgetDiversion = ((lo_block3 And &H40) >> 6) 'Y15 Fertig; 0=keine Umleitung , 1=Umleitung
        gGroupTMCgetDirection = ((lo_block3 And &H30) >> 4) 'Y14 Fertig;0=positive Richtung 1=negative Richtung
        gGroupTMCgetExtent = ((lo_block3 And &H38) >> 3) 'Y13,Y12,Y11 Fertig; Extent
        gGroupTMCgetEvent = (((lo_block3 And &H7) << 8) Or hi_block3) 'Y10-Y0 Fertig; Event
        gGroupTMCgetLocation = ((lo_block4 << 8) Or hi_block4) 'Z15-Z0 Fertig; Ort
        gGroupTMCgetFre = ((lo_block3 And &HF) << 24) Or (hi_block3 << 16) Or (lo_block4 << 8) Or (hi_block4)
        gGroupEreign1 = (lo_block3 And &H3) * 256 + hi_block3 */


        //  020d00020a 09         5bd5  0d           01              5c  1e      e0                  96 
        //  header     language?  loc   countryCode  locatonTableNo  dir extend  extendedCountryCode end
int32_t	DecoderDataAdapter::handle_tpeg_tmc (std::vector<uint8_t> temp_tpeg,uint16_t framebytes) {
  uint16_t location=0;
  uint16_t event=0;
  bool flag_location=false;
  // part location #####################################
         for (size_t i = 0; i < framebytes; i++) {
            if (i+6>=framebytes){
                return 0;
            }
            // ARD = vor location (02 0d 00 02 0a 09)
            if (((temp_tpeg[i]) == 0x0d) && ((temp_tpeg[i+1]) == 0x00) && ((temp_tpeg[i+2]) == 0x02) && ((temp_tpeg[i+3]) == 0x0a)&& ((temp_tpeg[i+4]) == 0x09)) {
               location = temp_tpeg[i+5]<<8;
               location= location +  temp_tpeg[i+6];
               flag_location=true;
             // fprintf(stderr, "\033[0;92m%02x \033[0;m", locevt); // location schreiben 
             }
            if (((temp_tpeg[i]) == 0x07) && ((temp_tpeg[i+6]) == 0x60) && (flag_location=true))
             {
               event = temp_tpeg[i+0x07]<<8; // 07 2c 2b 00 08 08 60 01 3c 00 07 60 01 3c 00 ????
               event = event + temp_tpeg[i+0x08];
               event =0;


               addOrReportDuplicate(DAB_TPEG_location, occurrences, location, event); // speichere Daten nur einmal in ein Array DAB_TPEG_location

                  // #################################################################
                  // tpeg_tmc_sqlite (location, event); // erst anzeigen wenn das zweite mal erscheint
                  // #################################################################
               location=0;
               event=0;
               flag_location=false;
            }
            
      }
      return 0;
}

static int sqliteLOCcallback(void *NotUsed, int argc, char **argv, char **azColName) {
   for(int i = 0; i < argc; i++) {
    //  fprintf(stderr,"%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
      std::string blob =  azColName[i];
      if (blob== "FIRST NAME")
         fprintf(stderr,"\033[0;33mNot real! %s = %s, \033[0;m", azColName[i], argv[i] ? argv[i] : "NULL");
      if (blob == "ROAD NAME"){
     //  if (!argv[i]) return 0; // null Strasse überlesen
         fprintf(stderr,"\033[0;91m%s = %s, \033[0;m", azColName[i], argv[i] ? argv[i] : "NULL");
      }
      if (blob== "ROAD NUMBER")
         fprintf(stderr,"\033[0;35m%s = %s, \033[0;m", azColName[i], argv[i] ? argv[i] : "NULL");

      if (blob== "POLIZEIDIENSTSTELLE")
         fprintf(stderr,"\033[0;31m%s = %s \033[0;m\n", azColName[i], argv[i] ? argv[i] : "NULL");
         
   }
      //fprintf(stderr,"\n");
   return 0;
}

static int sqliteEVTcallback(void *NotUsed, int argc, char **argv, char **azColName) {
   for(int i = 0; i < argc; i++) {
    //  fprintf(stderr,"%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
      std::string blob =  azColName[i];
      if (blob == "Text (German)"){
      // if (!argv[i]) return 0; // null Strasse überlesen
         fprintf(stderr,"\033[0;91m%s = %s, \033[0;m\n", azColName[i], argv[i] ? argv[i] : "NULL");
      }

         
   }
      //fprintf(stderr,"\n");
   return 0;
}


int32_t	DecoderDataAdapter::tpeg_tmc_sqlite (uint16_t location,uint16_t event) {
    sqlite3* db_location;
   sqlite3* db_event;
    char* errMsg = 0;

    int rc = sqlite3_open("/tmp/location.db", &db_location);
    if (rc) {
        std::cerr << "Kann Location Datenbank nicht öffnen: " << sqlite3_errmsg(db_location) << std::endl;
        return rc;
    }
 //   std::string sql = "SELECT \"ROAD NAME\" FROM DE_LCL_22 WHERE \"LOCATION CODE\" = \"" + std::to_string( locevtt) + "\";";
    std::string sql_loc = "SELECT \"FIRST NAME\", \"ROAD NAME\", \"ROAD NUMBER\",  \"POLIZEIDIENSTSTELLE\" FROM DE_LCL_22 WHERE \"LOCATION CODE\" = \"" + std::to_string( location) + "\";";
    rc = sqlite3_exec(db_location, sql_loc.c_str(), sqliteLOCcallback, 0, &errMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL-Fehler: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }

    sqlite3_close(db_location);
//  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    int rc_evt = sqlite3_open("/tmp/event.db", &db_event);
    if (rc_evt) {
        std::cerr << "Kann Event Datenbank nicht öffnen: " << sqlite3_errmsg(db_event) << std::endl;
        return rc_evt;
    }
 //   std::string sql = "SELECT \"ROAD NAME\" FROM DE_LCL_22 WHERE \"LOCATION CODE\" = \"" + std::to_string( locevtt) + "\";";
    std::string sql_evt = "SELECT \"Text (German)\" FROM \"ECL_Event List_DE_4\" WHERE \"CODE\" = \"" + std::to_string( event) + "\";";
    rc_evt = sqlite3_exec(db_event, sql_evt.c_str(), sqliteEVTcallback, 0, &errMsg);

    if (rc_evt != SQLITE_OK) {
        std::cerr << "SQL-Fehler: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }

    sqlite3_close(db_event);
    return 0;
}



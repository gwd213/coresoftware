#ifndef _EpdGeom
#define _EpdGeom

//#include "Rtypes.h"
#include "TVector3.h"

/*************************************
 * \author Mike Lisa
 * \date 4 Jan 2018
 *
 * \description:
 *  class defining geometrical aspects of an EPD Tile
 *  (position of center, RandomPointOnTile(), etc.)
 *
 * The user may pass the PP/TT/SN _or_ the uniqueID to
 *   most functions.  No option to pass EpdHit object,
 *   because we want to avoid StObject-dependence.
 *   Instead, simply use EpdHit::id(),
 *   e.g. RandomPointOnTile(hit->id())

 *
 *
 * Adapted for use in sPHENIX (from STAR)
 *   by Brennan Schaefer
 *   11 July 2022
 *
 *************************************/

class TRandom3;

class EpdGeom{
 private:
  short   mPP;  /// supersector position [1,12]
  short   mTT;  /// tile number on supersector [1,31]
  short   mSN;  /// North/South = +1/-1
  TRandom3* pRan;

  double mPhiCenter[12][31][2];  // PP,TT,SN
  double mRmin[16];   // row
  double mRmax[16];   // row
  double mRave[16];   // row

  // SouthNumberOfOverlappingBbcTiles[PP-1][TT-1] =  number of BBC tiles that overlap
  static short mSouthNumberOfOverlappingBbcTiles[12][9];  
  // SouthBbcTilesWhichOverlap[PP-1][TT-1][j] gives the BBC tile ID of the jth overlapping BBC tile
  static short mSouthBbcTilesWhichOverlap[12][9][3];  // 

  // NorthNumberOfOverlappingBbcTiles[PP-1][TT-1] =  number of BBC tiles that overlap
  static short mNorthNumberOfOverlappingBbcTiles[12][9];  
  // NorthBbcTilesWhichOverlap[PP-1][TT-1][j] gives the BBC tile ID of the jth overlapping BBC tile
  static short mNorthBbcTilesWhichOverlap[12][9][3];  // 


  void InitializeGeometry();

  /* these methods are used internally */
  /// z coordinate of the wheel in sPHENIX coordinate system
  /// depends on internal parameter mSN
  double GetZwheel();

  /// phi of the center of the tile in sPHENIX coordinate syste
  /// depends on internal parameters mPP, mTT, mSN
  //--------- obsolete (always the plan; that's why it was private)  double GetPhiCenter();
  /// the inner and outer extent of the tile in the radial direction
  /// depends on internal parameters mPP, mTT, mSN
  //--------- obsolete (always the plan; that's why it was private)  void     GetRminRmax(double *Rmin, double *Rmax);
  /// the "tile row" of the tile.  Row = [1,16]
  /// depends on internal parameter mTT
  short  Row();

  /// given the uniqueID of a tile (uniqueID = sign*(100*PP+TT) where sign=+1/-1 for North/South wheel
  /// this sets the internal parameters mPP, mTT, and mSN
  void     SetPpTtSn(short uniqueID);

  /// center of the tile in sPHENIX coordinate system
  /// depends on internal parameters mPP, mTT, mSN
  TVector3 TileCenter();

  /// returns a random point somewhere on the tile.
  /// assumes a uniform hit density
  /// this is very useful for calculating things like dN/deta
  /// depends on internal parameters mPP, mTT, mSN
  TVector3 RandomPointOnTile();

  /// returns the corners of the tile in the plane of the wheel, in sPHENIX coordinate system
  /// \param *nCorners   this is a RETURNED value. Number of corners the tile has (TT01 has 5, others have 4)
  /// \param x           this is a RETURNED values.  x-coordinates of corners
  /// \param y           this is a RETURNED values.  y-coordinates of corners
  /// depends on internal parameters mPP, mTT, mSN
  void     GetCorners(int* nCorners, double* x, double* y);

  /// returns true if (x,y) lies within the tile.  Assumes z=zWheel
  /// useful if the user would like to project a track (using straight line of helix or whatever)
  /// to the plane of the wheel and determine whether it hit a given tile
  /// \param x    x-coordinate of projected hit
  /// \param y    y-coordinate of projected hit
  /// depends on internal parameters mPP, mTT, mSN
  bool   IsInTile(double x, double y);

  /// returns a list of (the IDs of) BBC tiles that overlap with a given EPD tile
  /// \param nOverlappingBbcTiles         *output* parameter: number of BBC tiles that overlaps this EPD tile (even just barely)
  /// \param BbcTileIDs                   *output* parameter: array of BBC tile IDs
  void GetOverlappingBbcTiles(int* nOverlappingBbcTiles, short* BbcTileIDs);





 public:

  EpdGeom();
  ~EpdGeom();

  unsigned short position(short uniqueID);
  unsigned short tile(short uniqueID);


  /// center of the tile in sPHENIX coordinate system
  /// \param uniqueID    identifier of the tile = sign*(100*PP+TT) where sign=+/- for North/South
  TVector3 TileCenter(short uniqueID);

  /// center of the tile in sPHENIX coordinate system
  /// \param position   position of supersector [1,12]
  /// \param tilenumber tile on supsersector [1,31]
  /// \southnorth         south (-1) or north (+1) wheel
  TVector3 TileCenter(short position, short tilenumber, short southnorth);

  /// returns a random point somewhere on the tile.
  /// assumes a uniform hit density
  /// this is very useful for calculating things like dN/deta
  /// \param uniqueID    identifier of the tile = sign*(100*PP+TT) where sign=+/- for North/South
  TVector3 RandomPointOnTile(short uniqueID);

  /// returns a random point somewhere on the tile.
  /// assumes a uniform hit density
  /// this is very useful for calculating things like dN/deta
  /// \param position   position of supersector [1,12]
  /// \param tilenumber tile on supsersector [1,31]
  /// \southnorth         south (-1) or north (+1) wheel
  TVector3 RandomPointOnTile(short position, short tilenumber, short southnorth);

  /// returns the corners of the tile in the plane of the wheel, in sPHENIX coordinate system
  /// \param uniqueID    identifier of the tile = sign*(100*PP+TT) where sign=+/- for North/South
  /// \param *nCorners   this is a RETURNED value. Number of corners the tile has (TT01 has 5, others have 4)
  /// \param x           this is a RETURNED values.  x-coordinates of corners
  /// \param y           this is a RETURNED values.  y-coordinates of corners
  void GetCorners(short uniqueID, int* nCorners, double* x, double* y);
  /// returns the corners of the tile in the plane of the wheel, in sPHENIX coordinate system
  /// \param position   position of supersector [1,12]
  /// \param tilenumber tile on supsersector [1,31]
  /// \southnorth         south (-1) or north (+1) wheel
  /// \param *nCorners   this is a RETURNED value. Number of corners the tile has (TT01 has 5, others have 4)
  /// \param x           this is a RETURNED values.  x-coordinates of corners
  /// \param y           this is a RETURNED values.  y-coordinates of corners
  void GetCorners(short position, short tilenumber, short southnorth, int* nCorners, double* x, double* y);

  /// returns a list of (the IDs of) BBC tiles that overlap with a given EPD tile
  /// \param uniqueID                     identifier of the EPD tile = sign*(100*PP+TT) where sign=+/- for North/South
  /// \param nOverlappingBbcTiles         *output* parameter: number of BBC tiles that overlaps this EPD tile (even just barely)
  /// \param BbcTileIDs                   *output* parameter: array of BBC tile IDs
  void GetOverlappingBbcTiles(short uniqueID, int* nOverlappingBbcTiles, short* BbcTileIDs);

  /// returns a list of (the IDs of) BBC tiles that overlap with a given EPD tile
  /// \param position   position of supersector [1,12]
  /// \param tilenumber tile on supsersector [1,31]
  /// \southnorth         south (-1) or north (+1) wheel
  /// \param nOverlappingBbcTiles         *output* parameter: number of BBC tiles that overlaps this EPD tile (even just barely)
  /// \param BbcTileIDs                   *output* parameter: array of BBC tile IDs
  void GetOverlappingBbcTiles(short position, short tilenumber, short southnorth, int* nOverlappingBbcTiles, short* BbcTileIDs);



  /// returns true if (x,y) lies within the tile.  Assumes z=zWheel
  /// useful if the user would like to project a track (using straight line of helix or whatever)
  /// to the plane of the wheel and determine whether it hit a given tile
  /// \param uniqueID    identifier of the tile = sign*(100*PP+TT) where sign=+/- for North/South
  /// \param x    x-coordinate of projected hit
  /// \param y    y-coordinate of projected hit
  bool IsInTile(short uniqueID, double x, double y);

  /// returns true if (x,y) lies within the tile.  Assumes z=zWheel
  /// useful if the user would like to project a track (using straight line of helix or whatever)
  /// to the plane of the wheel and determine whether it hit a given tile
  /// \param position   position of supersector [1,12]
  /// \param tilenumber tile on supsersector [1,31]
  /// \southnorth         south (-1) or north (+1) wheel
  /// \param x    x-coordinate of projected hit
  /// \param y    y-coordinate of projected hit
  bool IsInTile(short position, short tilenumber, short southnorth, double x, double y);

  /// true if this tile is on north side
  /// \param uniqueID    identifier of the tile = sign*(100*PP+TT) where sign=+/- for North/South
  bool IsNorth(short uniqueID);

  /// true if this tile is on north side
  /// \param position   position of supersector [1,12]
  /// \param tilenumber tile on supsersector [1,31]
  /// \southnorth         south (-1) or north (+1) wheel
  bool IsNorth(short position, short tilenumber, short southnorth);

  /// true if this tile is on south side
  /// \param uniqueID    identifier of the tile = sign*(100*PP+TT) where sign=+/- for North/South
  bool IsSouth(short uniqueID);
  /// true if this tile is on south side
  /// \param position   position of supersector [1,12]
  /// \param tilenumber tile on supsersector [1,31]
  /// \southnorth         south (-1) or north (+1) wheel
  bool IsSouth(short position, short tilenumber, short southnorth);

  /// the "tile row" of the tile.  Row = [1,16]
  /// \param uniqueID    identifier of the tile = sign*(100*PP+TT) where sign=+/- for North/South
  short Row(short uniqueID);

  /// the "tile row" of the tile.  Row = [1,16]
  /// \param position   position of supersector [1,12]
  /// \param tilenumber tile on supsersector [1,31]
  /// \southnorth         south (-1) or north (+1) wheel
  short Row(short position, short tilenumber, short southnorth);

  ClassDef(EpdGeom,0)

};

inline bool EpdGeom::IsNorth(short uniqueID){return uniqueID>0;}
inline bool EpdGeom::IsNorth(short position, short tilenumber, short southnorth){return southnorth>0;}
inline bool EpdGeom::IsSouth(short uniqueID){return uniqueID<0;}
inline bool EpdGeom::IsSouth(short position, short tilenumber, short southnorth){return southnorth<0;}
inline unsigned short EpdGeom::position(short uniqueID){return abs(uniqueID/100);}
inline unsigned short EpdGeom::tile(short uniqueID){return abs(uniqueID%100);}




#endif

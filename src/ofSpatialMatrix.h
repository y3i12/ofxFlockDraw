//
//  ofSpatialMatrix.h
//  ofxFlockDraw
//
//  Created by Yuri Ivatchkovitch on 1/17/18.
//

#ifndef ofSpatialMatrix_h
#define ofSpatialMatrix_h

#include <vector>
#include "ofPoint.h"

// initial cell size must be get area / 4
template < typename T >
class spatial_matrix {
    typedef T                                   type_t;
    typedef spatial_matrix< type_t >            self_t;
    typedef std::vector< type_t* >              cell_t;
    typedef std::vector< cell_t >               matrix_t;
    
    typedef std::function< void ( self_t& matrix, type_t& a, type_t& b ) > radius_visitor_function_t;
    typedef std::function< void ( self_t& matrix, type_t& a ) >            visitor_function;
public:
    explicit spatial_matrix( float cell_radius, float field_width, float field_height )
    {
        resize( cell_radius, field_width, field_height );
    }
    
    explicit spatial_matrix( void ) : _c_r( 0.0f ), _f_w( 0.0f ), _f_h( 0.0f ), _w( 0 ), _h( 0 ) {}
    
    void resize( float cell_radius, float field_width, float field_height )
    {
        clear();
        _c_r = cell_radius;
        _f_w = field_width;
        _f_h = field_height;
        _w   = static_cast< size_t >( _f_w / _c_r ) + 1;
        _h   = static_cast< size_t >( _f_h / _c_r ) + 1;
        
        _cells.resize( _w * _h + 1);
    }
    
    void clear( void )
    {
        for ( auto& cell : _cells )
        {
            cell.clear();
        }
    }
    
    void insert( T& element, const ofPoint& position )
    {
        _get_cell( position ).push_back( &element );
    }
    
    void apply_to_radius( radius_visitor_function_t f, T& source_object, const ofPoint& position, float radius )
    {
        size_t d = static_cast< size_t >( ( radius     + radius ) / _c_r ) + 1;
        int _x   = static_cast< size_t >( ( position.x - radius ) / _c_r );
        int _y   = static_cast< size_t >( ( position.y - radius ) / _c_r );
        int _e_x = std::min< int >( _x + d, _w );
        int _e_y = std::min< int >( _y + d, _h );
        _x       = std::max< int >( _x, 0 );
        _y       = std::max< int >( _y, 0 );
        
        for ( int y = _y; y < _e_y; ++y )
        {
            for ( int x = _x; x < _e_x; ++x )
            {
                int i = x + y * _w;
                
                cell_t& c = _cells[ i ];
                for ( auto& e : c )
                {
                    f( *this, source_object, *e );
                }
            }
        }
    }
    
    void apply_to_all( visitor_function f )
    {
        _apply_to_all( _cells, f );
    }
    
    void clear_apply( visitor_function f )
    {
        matrix_t tmp_cells = _cells;
        clear();
        _apply_to_all( tmp_cells, f );
    }
    
    void resize_clear_apply( float cell_radius, visitor_function f )
    {
        resize( cell_radius, _f_w, _f_h );
        clear_apply( f );
    }
private:
    int       _get_index( const ofPoint& position )
    {
        return static_cast< size_t >( position.x / _c_r ) + ( static_cast< size_t >( position.y / _c_r ) * _w );
    }
    
    cell_t& _get_cell( const ofPoint& position )
    {
        return _cells[ _get_index( position ) ];
    }
    
    void _apply_to_all( matrix_t& m, visitor_function f )
    {
        for ( auto& c : m )
        {
            for ( auto& e : c )
            {
                f( *this, *e );
            }
        }
    }
    
private:
    matrix_t _cells;
    float _c_r; // cell width/height
    float _f_w; // field width
    float _f_h; // field height
    size_t _w;  // hrz cell num
    size_t _h;  // vrt cell num
};

#endif /* ofSpatialMatrix_h */

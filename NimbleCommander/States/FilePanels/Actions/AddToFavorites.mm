// Copyright (C) 2017 Michael Kazakov. Subject to GNU General Public License version 3.
#include <NimbleCommander/Bootstrap/AppDelegate.h>
#include "../Favorites.h"
#include "../FavoriteComposing.h"
#include "../PanelController.h"
#include "../PanelView.h"
#include "AddToFavorites.h"
#include "../PanelDataPersistency.h"

namespace nc::panel::actions {

bool AddToFavorites::Predicate( PanelController *_target ) const
{
    return _target.isUniform || _target.view.item;
}

void AddToFavorites::Perform( PanelController *_target, id _sender ) const
{
    auto &favorites = AppDelegate.me.favoriteLocationsStorage;
    if( auto item = _target.view.item ) {
        if( auto favorite = FavoriteComposing::FromListingItem(item) )
            favorites.AddFavoriteLocation( move(*favorite) );
    }
    else if( _target.isUniform )
        favorites.AddFavoriteLocation( *_target.vfs, _target.currentDirectoryPath );
}

};
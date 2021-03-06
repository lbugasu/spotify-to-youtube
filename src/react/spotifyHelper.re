open Js.Promise;

let clientId = "35988437b3404ab08d67eaced43c4997";

/* I'm going to be lazy and just cache this globally */
let myUserId: ref(option(string)) = ref(None);

let create = () =>
  Spotify.create({
    "clientId": clientId,
    "clientSecret": Js.Undefined.empty,
    "redirectUri": "http://localhost:54380/"
  });

let authUrl = (scope, state) => {
  let queryString =
    Qs.stringify({
      "client_id": clientId,
      "response_type": "token",
      "redirect_uri": "http://localhost:54380/",
      "state": state,
      "scope": String.concat(" ", scope)
    });
  "https://accounts.spotify.com/authorize?" ++ queryString
};

let loadMyUserId = (spotify) =>
  spotify##getMe()
  |> then_(
       (data) => {
         myUserId := Some(data##body##id);
         resolve(data##body##id)
       }
     );

let getMyUserId = (spotify) =>
  switch myUserId^ {
  | None => loadMyUserId(spotify)
  | Some(userId) => resolve(userId)
  };

let rec getUserPlaylistsPage = (spotify, offset, currentItems, userId) => {
  let limit = Spotify.maxPageSizePlaylists;
  let apiOptions = {"limit": Js.Undefined.return(limit), "offset": Js.Undefined.return(offset)};
  spotify##getUserPlaylists(userId, apiOptions)
  |> then_(
       (data) => {
         let combinedItems = Js.Array.concat(data##body##items, currentItems);
         let remainingItems = data##body##total - limit - offset;
         if (remainingItems > 0) {
           getUserPlaylistsPage(spotify, offset + limit, combinedItems, userId)
         } else {
           resolve(combinedItems)
         }
       }
     )
};

let getMyPlaylists = (spotify) =>
  getMyUserId(spotify) |> then_(getUserPlaylistsPage(spotify, 0, [||]));

let rec getPlaylistTracksByPage =
        (spotify: Js.t(Spotify.t), playlistId, playlistOwnerId, offset, currentItems, userId) => {
  let limit = Spotify.maxPageSizeTracks;
  let apiOptions = {
    "limit": Js.Undefined.return(limit),
    "offset": Js.Undefined.return(offset),
    "fields": Js.undefined
  };
  spotify##getPlaylistTracks(playlistOwnerId, playlistId, apiOptions)
  |> then_(
       (data) => {
         let combinedItems = Js.Array.concat(data##body##items, currentItems);
         let remainingItems = data##body##total - limit - offset;
         if (remainingItems > 0) {
           getPlaylistTracksByPage(spotify, playlistId, playlistOwnerId, offset + limit, combinedItems, userId)
         } else {
           resolve(combinedItems)
         }
       }
     )
};

let getPlaylistTracks = (spotify: Js.t(Spotify.t), playlistId, playlistOwnerId) =>
  getMyUserId(spotify) |> then_(getPlaylistTracksByPage(spotify, playlistId, playlistOwnerId, 0, [||]));

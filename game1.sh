function netListen {
  nc -l $port
}

function toNet {
  echo $1 | nc $host $port
}

function tryConnect {
  #try to establish connection with initial mark
  mark="x"
  opponent_mark="o"
  connected=0
  toNet $mark

  if [ $? == 0 ]; then
    #there is an opponent, connection is established
    connected=1
    mark="o"
    opponent_mark="x"
    newGame
  else
    echo "Waiting for connection"
    connected=1
    mark=$(netListen)
    newGame
  fi
}

function sendCommand {
  read L
  processCommand $mark $L
  toNet $L
  if [ $? != 0 ]; then
    #connection is broken, game is over
    connected=0
    valid_turn=0
    tryConnect
  fi
}

function receiveCommand {
  code=$(netListen)
  if [ $code == "x" ]; then
    #connection was broken, now start new connection and new game
    mark="x"
    opponent_mark="o"
    newGame
    valid_turn=0
  else
    processCommand $opponent_mark $code
  fi
}

#drawing
function redraw {
  tput clear
  tput cup 0 0

  printf "    1 | 2 | 3\n"
  printf "  -------------\n"
  for i in `seq 0 2`; do
    printf "%-2s| %-2s| %-2s| %-2s|\n" $(($i+1)) ${field[$i*3]} ${field[$i*3+1]} ${field[$i*3+2]}
    printf "  -------------\n"
  done
  printf "You are %s\n" $mark
}

#logic
function newGame {
  for i in `seq 0 9`; do
    field[$i]="_"
  done
  redraw

  current_turn="x"
}

function changeTurn {
  if [ $current_turn == "x" ]; then
    current_turn="o"
  else
    current_turn="x"
  fi
}

function processCommand {
  local mark=$1
  local comm=$2
  valid_turn=1

  #process command
  if [ ${#comm} -ne 2 ]; then
    valid_turn=0
    return
  fi

  local x="$(echo $comm | head -c 1)"
  local y="$(echo $comm | tail -c 2)"
  if [ -z $x ] || [ -z $y ]; then
    valid_turn=0
    return
  fi

  local re='^[0-9]+$'
  if ! [[ $x =~ $re ]] || ! [[ $y =~ $re ]]; then
    valid_turn=0
    return
  fi

  if [ $x -lt 0 ] || [ $y -lt 0 ]; then
    valid_turn=0
    return
  fi 2>/dev/null

  offset="$((($x-1)*3+$y-1))"
  if [ $x -le 3 ] && [ $y -le 3 ] && [ ${field[$offset]} == "_" ]; then
    field[$offset]=$mark
    valid_turn=1
  else
    valid_turn=0
  fi 2>/dev/null
}

function checkIfGameOver {
  local row_count_x=0
  local row_count_o=0
  local col_count_x=0
  local col_count_o=0
  local offset
  allOccupied=1

  for row in `seq 0 2`; do
    row_count_x=0
    row_count_o=0
    col_count_x=0
    col_count_o=0
    for col in `seq 0 2`; do
      offset=$(($row*3+$col))
      if [ ${field[$offset]} != "_" ]; then
        if [ ${field[$offset]} == "x" ]; then
          row_count_x=$(($row_count_x + 1))
        else
          row_count_o=$(($row_count_o + 1))
        fi
      else
        allOccupied=0
      fi
      offset=$(($col*3+$row))
      if [ ${field[$offset]} != "_" ]; then
        if [ ${field[$offset]} == "x" ]; then
          col_count_x=$(($col_count_x + 1))
        else
          col_count_o=$(($col_count_o + 1))
        fi
      else
        allOccupied=0
      fi
    done

    if [ $row_count_x == 3 ] || [ $col_count_x == 3 ]; then
      game_over="x"
      return
    fi
    if [ $row_count_o == 3 ] || [ $col_count_o == 3 ]; then
      game_over="o"
      return
    fi
  done

  if [ $allOccupied == 1 ]; then
    return
  fi

  row_count_x=0
  row_count_o=0
  col_count_x=0
  col_count_o=0
  for row in `seq 0 2`; do
    offset=$(($row*3+$row))
    if [ ${field[$offset]} != "_" ]; then
      if [ ${field[$offset]} == "x" ]; then
        row_count_x=$(($row_count_x + 1))
      else
        row_count_o=$(($row_count_o + 1))
      fi
    fi
    offset=$(($row*3+2-$row))
    if [ ${field[$offset]} != "_" ]; then
      if [ ${field[$offset]} == "x" ]; then
        col_count_x=$(($col_count_x + 1))
      else
        col_count_o=$(($col_count_o + 1))
      fi
    fi
  done
  if [ $row_count_x == 3 ] || [ $col_count_x == 3 ]; then
    game_over="x"
    return
  fi
  if [ $row_count_o == 3 ] || [ $col_count_o == 3 ]; then
    game_over="o"
    return
  fi
}

# get host and port (parametrized) or print help if needed
if [ $# -ne 2 ]; then
  if [ $# -eq 1 ] && ([ $1 == "--help" ] || [ $1 == "-h" ]); then
    echo "$0"
    echo "Commands: xy"
    echo "1 <= x <= 3"
    echo "2 <= y <= 3"
  else
    echo "Not enough arguments"
    echo "Usage: $0 host-of-opponent port"
  fi
  exit 1
fi

host=$1
port=$2

connected=0             # connection monitor, if connection is broken, need to reset all
valid_turn=1            # turn validation monitor, if = 0, do not need next iteration of the game
game_over="_"           # game over monitor, containts winner's mark
allOccupied=0           # field is occupied monitor, if = 1, game should be finished because there is no possible turns although there is no winner
mark="x"                # player's mark, by default = x (first player)
opponent_mark="o"       # player's opponent's mark, by default = o (second player)

declare field
tryConnect

current_turn="x"
while [ $game_over == "_" ] && [ $allOccupied == 0 ]; do
  if [ $current_turn == $mark ]; then
    echo "Your turn"
    sendCommand
  else
    echo "Waiting for opponent's turn"
    receiveCommand
  fi

  if [ $connected == 1 ] && [ $valid_turn == 1 ]; then
    redraw
    changeTurn
  else
    echo "Connection is broken or invalid turn"
  fi
  checkIfGameOver
done

if [ $game_over == "_" ]; then
  echo "Game over: to possible turns"
  exit 0
fi

if [ $game_over == $mark ]; then
  echo "You won"
else
  echo "Opponent won"
fi

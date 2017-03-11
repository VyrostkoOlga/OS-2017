#!/bin/bash
#
# Commands: xy

function tryConnect {
  echo $mark | nc $host $port
  if [ $? == 0 ]; then
    connected=1
    mark="x"
    other_mark="o"
  else
    mark="o"
    other_mark="x"
  fi
}

function toNet {
  read L
  processCommand $mark $L
  echo $L | nc $host $port
}

function NetListen {
  nc -l $port
}

function fromNet {
  code=$(NetListen)
  if [ $code == $mark ]; then
    connected=1
    return
  fi

  processCommand $other_mark $code
}

function processCommand {
  local mark=$1
  local comm=$2

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

function redraw {
  tput clear
  tput cup 0 0

  printf "    1 | 2 | 3\n"
  printf "  -------------\n"
  for i in `seq 0 2`; do
    printf "%-2s| %-2s| %-2s| %-2s|\n" $(($i+1)) ${field[$i*3]} ${field[$i*3+1]} ${field[$i*3+2]}
    printf "  -------------\n"
  done
}

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
connected=0
mark="o"
other_mark="x"
game_over="_"
valid_turn=1
turn=1
allOccupied=0

tryConnect

declare field
for i in `seq 0 9`; do
  field[$i]="_"
done
redraw

current_turn=0
while [ true ]; do
  checkIfGameOver
  if [ $allOccupied == 1 ]; then
    echo "No possible turns"
    break
  fi

  if [ $game_over != "_" ]; then
    echo "Game over"
    if [ $game_over == $mark ]; then
      echo "You won"
    else
      echo "Your opponent won"
    fi
    break
  fi

  if [ $turn == $current_turn ]; then
    echo "Your turn"
    toNet
  else
    if [ $connected == 0 ]; then
      echo "Waiting for the connection"
    else
      echo "Waiting for the turn"
    fi
    fromNet
  fi
  if [ $valid_turn == 1 ]; then
    current_turn=$((!$current_turn))
    redraw
  else
    if [ $turn == $current_turn ]; then
      echo "Invalid turn"
    fi
  fi
done
